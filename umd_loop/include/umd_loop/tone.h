/*
 * Copyright (c) 2026 UMD Loop
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UMD_LOOP_TONE_H_
#define UMD_LOOP_TONE_H_

#include <zephyr/device.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Play a square-wave tone at @p freq_hz for @p duration_ms using PWM.
 * Pass duration_ms == 0 to use the ADC-mapped duration (E1 System 3).
 */
int tone_play(const struct device *dev, uint32_t freq_hz, uint32_t duration_ms);

/**
 * Read the ADC sensor and map 0..full-scale to min/max duration (ms).
 * Returns duration in ms, or negative errno.
 */
int tone_adc_duration(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* UMD_LOOP_TONE_H_ */
