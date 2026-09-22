/*
 * Copyright (c) 2026 UMD Loop
 * SPDX-License-Identifier: Apache-2.0
 *
 * Port of E1 System 1 RGB output (hexanypin.ino).
 */

#define DT_DRV_COMPAT umdloop_rgb_led

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <errno.h>

#include <umd_loop/rgb_led.h>

LOG_MODULE_REGISTER(umd_loop_rgb_led, CONFIG_LOG_DEFAULT_LEVEL);

struct rgb_led_config {
	struct gpio_dt_spec red;
	struct gpio_dt_spec green;
	struct gpio_dt_spec blue;
};

static const struct gpio_dt_spec *channel_spec(const struct rgb_led_config *cfg,
					      enum rgb_led_channel ch)
{
	switch (ch) {
	case RGB_LED_RED:
		return &cfg->red;
	case RGB_LED_GREEN:
		return &cfg->green;
	case RGB_LED_BLUE:
		return &cfg->blue;
	default:
		return NULL;
	}
}

int rgb_led_all_off(const struct device *dev)
{
	const struct rgb_led_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	ret = gpio_pin_set_dt(&cfg->red, 0);
	if (ret) {
		return ret;
	}
	ret = gpio_pin_set_dt(&cfg->green, 0);
	if (ret) {
		return ret;
	}
	return gpio_pin_set_dt(&cfg->blue, 0);
}

int rgb_led_set_channel(const struct device *dev, enum rgb_led_channel ch, bool on)
{
	const struct rgb_led_config *cfg = dev->config;
	const struct gpio_dt_spec *spec = channel_spec(cfg, ch);

	if (!device_is_ready(dev) || spec == NULL) {
		return -EINVAL;
	}

	return gpio_pin_set_dt(spec, on ? 1 : 0);
}

static int64_t dist2(int r, int g, int b, int tr, int tg, int tb)
{
	int64_t dr = r - tr;
	int64_t dg = g - tg;
	int64_t db = b - tb;

	return dr * dr + dg * dg + db * db;
}

int rgb_led_nearest_primary(const struct device *dev, uint8_t r, uint8_t g, uint8_t b)
{
	int64_t d_r = dist2(r, g, b, 255, 0, 0);
	int64_t d_g = dist2(r, g, b, 0, 255, 0);
	int64_t d_b = dist2(r, g, b, 0, 0, 255);
	enum rgb_led_channel winner;
	int ret;

	ret = rgb_led_all_off(dev);
	if (ret) {
		return ret;
	}

	if (d_r <= d_g && d_r <= d_b) {
		winner = RGB_LED_RED;
		LOG_INF("closest color: red (R=%u G=%u B=%u)", r, g, b);
	} else if (d_g <= d_r && d_g <= d_b) {
		winner = RGB_LED_GREEN;
		LOG_INF("closest color: green (R=%u G=%u B=%u)", r, g, b);
	} else {
		winner = RGB_LED_BLUE;
		LOG_INF("closest color: blue (R=%u G=%u B=%u)", r, g, b);
	}

	return rgb_led_set_channel(dev, winner, true);
}

static int rgb_led_init(const struct device *dev)
{
	const struct rgb_led_config *cfg = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&cfg->red) || !gpio_is_ready_dt(&cfg->green) ||
	    !gpio_is_ready_dt(&cfg->blue)) {
		LOG_ERR("RGB GPIOs not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->red, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}
	ret = gpio_pin_configure_dt(&cfg->green, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}
	ret = gpio_pin_configure_dt(&cfg->blue, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}

	return 0;
}

#define RGB_LED_DEFINE(n)                                                      \
	static const struct rgb_led_config rgb_led_cfg_##n = {                 \
		.red = GPIO_DT_SPEC_INST_GET(n, red_gpios),                    \
		.green = GPIO_DT_SPEC_INST_GET(n, green_gpios),                \
		.blue = GPIO_DT_SPEC_INST_GET(n, blue_gpios),                  \
	};                                                                     \
	DEVICE_DT_INST_DEFINE(n, rgb_led_init, NULL, NULL, &rgb_led_cfg_##n,   \
			      POST_KERNEL, CONFIG_UMD_LOOP_RGB_LED_INIT_PRIORITY, \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(RGB_LED_DEFINE)
