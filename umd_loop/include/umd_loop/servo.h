/*
 * Copyright (c) 2026 UMD Loop
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UMD_LOOP_SERVO_H_
#define UMD_LOOP_SERVO_H_

#include <zephyr/device.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Move servo to @p degrees (clamped to 0..180). */
int servo_set_angle(const struct device *dev, int degrees);

/** Move servo to 0 degrees (E1 home position). */
int servo_home(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* UMD_LOOP_SERVO_H_ */
