/*
 * Copyright (c) 2026 UMD Loop
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UMD_LOOP_RGB_LED_H_
#define UMD_LOOP_RGB_LED_H_

#include <zephyr/device.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum rgb_led_channel {
	RGB_LED_RED = 0,
	RGB_LED_GREEN,
	RGB_LED_BLUE,
};

int rgb_led_all_off(const struct device *dev);
int rgb_led_set_channel(const struct device *dev, enum rgb_led_channel ch, bool on);

/**
 * Light the single primary color (R/G/B) closest to the given RGB triple
 * using squared Euclidean distance (E1 System 1 behavior).
 */
int rgb_led_nearest_primary(const struct device *dev, uint8_t r, uint8_t g, uint8_t b);

#ifdef __cplusplus
}
#endif

#endif /* UMD_LOOP_RGB_LED_H_ */
