/*
 * Copyright (c) 2026 UMD Loop
 * SPDX-License-Identifier: Apache-2.0
 *
 * Port of E1 System 3 (tone + ADC duration).
 */

#define DT_DRV_COMPAT umdloop_tone

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <errno.h>

#include <umd_loop/tone.h>

LOG_MODULE_REGISTER(umd_loop_tone, CONFIG_LOG_DEFAULT_LEVEL);

struct tone_config {
	struct pwm_dt_spec pwm;
	bool has_adc;
	struct adc_dt_spec adc;
	uint32_t min_duration_ms;
	uint32_t max_duration_ms;
};

int tone_adc_duration(const struct device *dev)
{
	const struct tone_config *cfg = dev->config;
	uint16_t sample = 0;
	struct adc_sequence sequence = {
		.buffer = &sample,
		.buffer_size = sizeof(sample),
	};
	int32_t raw;
	int32_t full;
	uint32_t span;
	uint32_t duration;
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}
	if (!cfg->has_adc) {
		return (int)cfg->min_duration_ms;
	}

	ret = adc_sequence_init_dt(&cfg->adc, &sequence);
	if (ret) {
		return ret;
	}

	ret = adc_read_dt(&cfg->adc, &sequence);
	if (ret) {
		return ret;
	}

	raw = sample;
	full = (1 << cfg->adc.resolution) - 1;
	if (full <= 0) {
		full = 4095;
	}
	if (raw < 0) {
		raw = 0;
	} else if (raw > full) {
		raw = full;
	}

	span = cfg->max_duration_ms - cfg->min_duration_ms;
	duration = cfg->min_duration_ms + (span * (uint32_t)raw) / (uint32_t)full;
	LOG_INF("ADC raw=%d duration=%u ms", raw, duration);
	return (int)duration;
}

int tone_play(const struct device *dev, uint32_t freq_hz, uint32_t duration_ms)
{
	const struct tone_config *cfg = dev->config;
	uint32_t period_ns;
	uint32_t pulse_ns;
	int ret;

	if (!device_is_ready(dev) || freq_hz == 0U) {
		return -EINVAL;
	}

	if (duration_ms == 0U) {
		ret = tone_adc_duration(dev);
		if (ret < 0) {
			return ret;
		}
		duration_ms = (uint32_t)ret;
	}

	period_ns = 1000000000U / freq_hz;
	pulse_ns = period_ns / 2U;

	LOG_INF("tone %u Hz for %u ms", freq_hz, duration_ms);
	ret = pwm_set_dt(&cfg->pwm, period_ns, pulse_ns);
	if (ret) {
		return ret;
	}

	k_msleep(duration_ms);

	/* Stop tone (0% duty). */
	return pwm_set_dt(&cfg->pwm, period_ns, 0);
}

static int tone_init(const struct device *dev)
{
	const struct tone_config *cfg = dev->config;
	int ret;

	if (!pwm_is_ready_dt(&cfg->pwm)) {
		LOG_ERR("tone PWM not ready");
		return -ENODEV;
	}

	if (cfg->has_adc) {
		if (!adc_is_ready_dt(&cfg->adc)) {
			LOG_ERR("tone ADC not ready");
			return -ENODEV;
		}
		ret = adc_channel_setup_dt(&cfg->adc);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

#define TONE_HAS_ADC(n) DT_INST_NODE_HAS_PROP(n, io_channels)

#define TONE_DEFINE(n)                                                         \
	static const struct tone_config tone_cfg_##n = {                       \
		.pwm = PWM_DT_SPEC_INST_GET(n),                                \
		.has_adc = TONE_HAS_ADC(n),                                    \
		.adc = COND_CODE_1(TONE_HAS_ADC(n),                            \
				   (ADC_DT_SPEC_INST_GET_BY_IDX(n, 0)),        \
				   ({0})),                                     \
		.min_duration_ms = DT_INST_PROP(n, min_duration_ms),           \
		.max_duration_ms = DT_INST_PROP(n, max_duration_ms),           \
	};                                                                     \
	DEVICE_DT_INST_DEFINE(n, tone_init, NULL, NULL, &tone_cfg_##n,         \
			      POST_KERNEL, CONFIG_UMD_LOOP_TONE_INIT_PRIORITY, \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(TONE_DEFINE)
