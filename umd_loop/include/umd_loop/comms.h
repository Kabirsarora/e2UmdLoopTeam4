/*
 * Copyright (c) 2026 UMD Loop
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UMD_LOOP_COMMS_H_
#define UMD_LOOP_COMMS_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Parsed 24-bit color from a HEX line (#RRGGBB / 0xRRGGBB / RRGGBB). */
struct comms_rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

/**
 * Parse a HEX color string (optional # / 0x prefix, exactly 6 hex digits).
 * Returns 0 on success, -EINVAL on bad format.
 */
int comms_parse_hex(const char *text, struct comms_rgb *out);

/**
 * Read one newline-terminated line from @p uart into @p buf.
 * Returns length of line (excluding terminator), 0 if incomplete, or negative errno.
 */
int comms_uart_readline(const struct device *uart, char *buf, size_t buflen,
			size_t *accum);

/** Blink the status LED @p times (USB_comm / I2C_receiver pattern). */
int comms_status_blink(const struct device *gpio_dev, gpio_pin_t pin,
		       gpio_flags_t flags, unsigned times, int on_ms, int off_ms);

/**
 * I2C slave helper: push received bytes into @p msgq from the ISR/callback
 * context. App threads block on k_msgq_get.
 */
int comms_i2c_msgq_put(struct k_msgq *msgq, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* UMD_LOOP_COMMS_H_ */
