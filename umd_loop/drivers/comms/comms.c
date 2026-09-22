/*
 * Copyright (c) 2026 UMD Loop
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared protocol helpers for HEX UART (System 1), USB demux, I2C blink.
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <umd_loop/comms.h>

int comms_parse_hex(const char *text, struct comms_rgb *out)
{
	char buf[16];
	size_t len;
	size_t i;
	unsigned long v;
	char *end;

	if (text == NULL || out == NULL) {
		return -EINVAL;
	}

	/* Trim leading whitespace */
	while (*text == ' ' || *text == '\t') {
		text++;
	}

	len = strlen(text);
	while (len > 0 && (text[len - 1] == ' ' || text[len - 1] == '\t' ||
			   text[len - 1] == '\r' || text[len - 1] == '\n')) {
		len--;
	}

	if (len >= sizeof(buf)) {
		return -EINVAL;
	}
	memcpy(buf, text, len);
	buf[len] = '\0';

	if (buf[0] == '#') {
		memmove(buf, buf + 1, len);
		len--;
	} else if (len >= 2 && buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X')) {
		memmove(buf, buf + 2, len - 1);
		len -= 2;
	}

	if (len != 6) {
		return -EINVAL;
	}
	for (i = 0; i < 6; i++) {
		if (!isxdigit((unsigned char)buf[i])) {
			return -EINVAL;
		}
	}

	v = strtoul(buf, &end, 16);
	if (end == buf) {
		return -EINVAL;
	}

	out->r = (uint8_t)((v >> 16) & 0xFF);
	out->g = (uint8_t)((v >> 8) & 0xFF);
	out->b = (uint8_t)(v & 0xFF);
	return 0;
}

int comms_uart_readline(const struct device *uart, char *buf, size_t buflen,
			size_t *accum)
{
	unsigned char c;
	int ret;

	if (uart == NULL || buf == NULL || accum == NULL || buflen < 2) {
		return -EINVAL;
	}
	if (!device_is_ready(uart)) {
		return -ENODEV;
	}

	while (true) {
		ret = uart_poll_in(uart, &c);
		if (ret == -1) {
			/* No data yet */
			return 0;
		}
		if (ret < 0) {
			return ret;
		}

		if (c == '\n' || c == '\r') {
			if (*accum > 0) {
				buf[*accum] = '\0';
				ret = (int)*accum;
				*accum = 0;
				return ret;
			}
			continue;
		}

		if (*accum + 1 >= buflen) {
			*accum = 0;
			return -ENOMEM;
		}
		buf[(*accum)++] = (char)c;
	}
}

int comms_status_blink(const struct device *gpio_dev, gpio_pin_t pin,
		       gpio_flags_t flags, unsigned times, int on_ms, int off_ms)
{
	unsigned i;
	int ret;

	if (gpio_dev == NULL || !device_is_ready(gpio_dev)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure(gpio_dev, pin, GPIO_OUTPUT_INACTIVE | flags);
	if (ret) {
		return ret;
	}

	for (i = 0; i < times; i++) {
		gpio_pin_set(gpio_dev, pin, 1);
		k_msleep(on_ms);
		gpio_pin_set(gpio_dev, pin, 0);
		k_msleep(off_ms);
	}
	return 0;
}

int comms_i2c_msgq_put(struct k_msgq *msgq, uint8_t value)
{
	if (msgq == NULL) {
		return -EINVAL;
	}
	return k_msgq_put(msgq, &value, K_NO_WAIT);
}
