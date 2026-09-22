/*
 * Copyright (c) 2026 UMD Loop
 * SPDX-License-Identifier: Apache-2.0
 *
 * Port of E1 System 2 (E1_System_2) using Zephyr PWM.
 */

#define DT_DRV_COMPAT umdloop_servo_pwm

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include <errno.h>

#include <umd_loop/servo.h>

LOG_MODULE_REGISTER(umd_loop_servo, CONFIG_LOG_DEFAULT_LEVEL);

#define SERVO_PERIOD_NS 20000000U /* 50 Hz */

struct servo_config {
	struct pwm_dt_spec pwm;
	uint32_t min_pulse_us;
	uint32_t max_pulse_us;
};

static uint32_t angle_to_pulse_ns(const struct servo_config *cfg, int degrees)
{
	uint32_t span;
	uint32_t pulse_us;

	if (degrees < 0) {
		degrees = 0;
	} else if (degrees > 180) {
		degrees = 180;
	}

	span = cfg->max_pulse_us - cfg->min_pulse_us;
	pulse_us = cfg->min_pulse_us + (span * (uint32_t)degrees) / 180U;
	return pulse_us * 1000U;
}

int servo_set_angle(const struct device *dev, int degrees)
{
	const struct servo_config *cfg = dev->config;
	uint32_t pulse_ns;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	pulse_ns = angle_to_pulse_ns(cfg, degrees);
	LOG_INF("servo angle %d -> pulse %u ns", degrees, pulse_ns);
	return pwm_set_dt(&cfg->pwm, SERVO_PERIOD_NS, pulse_ns);
}

int servo_home(const struct device *dev)
{
	return servo_set_angle(dev, 0);
}

static int servo_init(const struct device *dev)
{
	const struct servo_config *cfg = dev->config;

	if (!pwm_is_ready_dt(&cfg->pwm)) {
		LOG_ERR("servo PWM not ready");
		return -ENODEV;
	}

	return servo_home(dev);
}

#define SERVO_DEFINE(n)                                                        \
	static const struct servo_config servo_cfg_##n = {                     \
		.pwm = PWM_DT_SPEC_INST_GET(n),                                \
		.min_pulse_us = DT_INST_PROP(n, min_pulse_us),                 \
		.max_pulse_us = DT_INST_PROP(n, max_pulse_us),                 \
	};                                                                     \
	DEVICE_DT_INST_DEFINE(n, servo_init, NULL, NULL, &servo_cfg_##n,       \
			      POST_KERNEL, CONFIG_UMD_LOOP_SERVO_INIT_PRIORITY, \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(SERVO_DEFINE)
