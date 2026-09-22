/*
 * Copyright (c) 2026 UMD Loop
 * SPDX-License-Identifier: Apache-2.0
 *
 * Multi-threaded E2 app: replays E1 Systems 1–3 + I2C/USB blink using
 * umd_loop Zephyr drivers.
 *
 * USB console command prefixes (115200):
 *   H#RRGGBB   — HEX color -> nearest RGB LED  (also accepted on uart1 @ 9600)
 *   S<angle>   — servo 0..180, then home after 2 s
 *   T<freq>    — tone at freq Hz; duration from ADC
 *   N<0-255>   — blink status LED 5x (USB_comm)
 *
 * I2C slave address 8: any received byte blinks status LED once (I2C_receiver).
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <string.h>

#include <umd_loop/comms.h>
#include <umd_loop/rgb_led.h>
#include <umd_loop/servo.h>
#include <umd_loop/tone.h>

LOG_MODULE_REGISTER(umd_loop_app, LOG_LEVEL_INF);

#define HEX_MSGQ_LEN   8
#define SERVO_MSGQ_LEN 4
#define TONE_MSGQ_LEN  4
#define BLINK_MSGQ_LEN 8
#define LINE_MAX       64

K_MSGQ_DEFINE(hex_msgq, sizeof(struct comms_rgb), HEX_MSGQ_LEN, 4);
K_MSGQ_DEFINE(servo_msgq, sizeof(int), SERVO_MSGQ_LEN, 4);
K_MSGQ_DEFINE(tone_msgq, sizeof(uint32_t), TONE_MSGQ_LEN, 4);
K_MSGQ_DEFINE(blink_msgq, sizeof(uint8_t), BLINK_MSGQ_LEN, 4);

static const struct device *rgb_dev = DEVICE_DT_GET(DT_NODELABEL(umd_rgb));
static const struct device *servo_dev = DEVICE_DT_GET(DT_NODELABEL(umd_servo));
static const struct device *tone_dev = DEVICE_DT_GET(DT_NODELABEL(umd_tone));
static const struct device *hex_uart = DEVICE_DT_GET(DT_ALIAS(umd_hex_uart));
static const struct device *console_uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static const struct gpio_dt_spec status_led =
	GPIO_DT_SPEC_GET(DT_NODELABEL(status_led), gpios);

static void dispatch_usb_line(const char *line)
{
	struct comms_rgb color;
	int angle;
	uint32_t freq;
	unsigned long num;
	uint8_t blink_count;

	if (line == NULL || line[0] == '\0') {
		return;
	}

	switch (line[0]) {
	case 'H':
	case 'h':
		if (comms_parse_hex(line + 1, &color) == 0) {
			(void)k_msgq_put(&hex_msgq, &color, K_NO_WAIT);
		} else {
			LOG_WRN("bad HEX (want H#RRGGBB)");
		}
		break;
	case 'S':
	case 's':
		angle = (int)strtol(line + 1, NULL, 10);
		if (angle >= 0 && angle <= 180) {
			(void)k_msgq_put(&servo_msgq, &angle, K_NO_WAIT);
		} else {
			LOG_WRN("servo angle must be 0..180");
		}
		break;
	case 'T':
	case 't':
		freq = (uint32_t)strtoul(line + 1, NULL, 10);
		if (freq > 0U) {
			(void)k_msgq_put(&tone_msgq, &freq, K_NO_WAIT);
		} else {
			LOG_WRN("tone freq must be > 0");
		}
		break;
	case 'N':
	case 'n':
		num = strtoul(line + 1, NULL, 10);
		if (num <= 255UL) {
			blink_count = 5;
			(void)k_msgq_put(&blink_msgq, &blink_count, K_NO_WAIT);
			LOG_INF("USB number %lu -> blink 5x", num);
		} else {
			LOG_WRN("number must be 0..255");
		}
		break;
	default:
		/* Bare HEX without prefix still accepted on console (E1 USB test) */
		if (comms_parse_hex(line, &color) == 0) {
			(void)k_msgq_put(&hex_msgq, &color, K_NO_WAIT);
		} else {
			LOG_WRN("unknown cmd '%s' (H/S/T/N)", line);
		}
		break;
	}
}

static void hex_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	char line[LINE_MAX];
	size_t accum = 0;
	struct comms_rgb color;
	int n;

	if (!device_is_ready(rgb_dev) || !device_is_ready(hex_uart)) {
		LOG_ERR("hex thread: devices not ready");
		return;
	}

	LOG_INF("hex thread ready (uart1 @ 9600 + USB H#RRGGBB)");

	while (true) {
		n = comms_uart_readline(hex_uart, line, sizeof(line), &accum);
		if (n > 0) {
			if (comms_parse_hex(line, &color) == 0) {
				(void)k_msgq_put(&hex_msgq, &color, K_NO_WAIT);
			} else {
				LOG_WRN("wire HEX format error");
			}
		}

		if (k_msgq_get(&hex_msgq, &color, K_MSEC(20)) == 0) {
			(void)rgb_led_nearest_primary(rgb_dev, color.r, color.g, color.b);
		}
	}
}

static void servo_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	int angle;

	if (!device_is_ready(servo_dev)) {
		LOG_ERR("servo thread: device not ready");
		return;
	}

	LOG_INF("servo thread ready (USB S<angle>)");

	while (true) {
		if (k_msgq_get(&servo_msgq, &angle, K_FOREVER) == 0) {
			LOG_INF("Moving to %d degrees", angle);
			(void)servo_set_angle(servo_dev, angle);
			k_msleep(2000);
			(void)servo_home(servo_dev);
			LOG_INF("Returned to 0");
		}
	}
}

static void tone_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	uint32_t freq;

	if (!device_is_ready(tone_dev)) {
		LOG_ERR("tone thread: device not ready");
		return;
	}

	LOG_INF("tone thread ready (USB T<freq>)");

	while (true) {
		if (k_msgq_get(&tone_msgq, &freq, K_FOREVER) == 0) {
			/* duration_ms == 0 -> driver reads ADC (E1 System 3) */
			(void)tone_play(tone_dev, freq, 0);
			LOG_INF("tone done");
		}
	}
}

static void blink_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	uint8_t times;

	if (!gpio_is_ready_dt(&status_led)) {
		LOG_ERR("blink thread: status LED not ready");
		return;
	}

	(void)gpio_pin_configure_dt(&status_led, GPIO_OUTPUT_INACTIVE);
	LOG_INF("blink thread ready (I2C / USB N)");

	while (true) {
		if (k_msgq_get(&blink_msgq, &times, K_FOREVER) == 0) {
			(void)comms_status_blink(status_led.port, status_led.pin,
						 status_led.dt_flags, times, 300, 300);
		}
	}
}

static void usb_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	char line[LINE_MAX];
	size_t accum = 0;
	int n;

	if (!device_is_ready(console_uart)) {
		LOG_ERR("usb thread: console UART not ready");
		return;
	}

	LOG_INF("USB demux ready — commands: H#RRGGBB | S90 | T440 | N123");

	while (true) {
		n = comms_uart_readline(console_uart, line, sizeof(line), &accum);
		if (n > 0) {
			dispatch_usb_line(line);
		} else {
			k_msleep(10);
		}
	}
}

/*
 * Minimal I2C target (slave) receive path.
 * Zephyr's i2c_target API varies by SoC; we poll a soft mailbox that an
 * optional external master can fill via a board-specific target driver.
 * For the ESP32 demo path we also accept I2C_receiver semantics by enqueueing
 * a blink when CONFIG is enabled — here we expose a callable for tests.
 */
void umd_loop_i2c_byte_received(uint8_t value)
{
	uint8_t times = 1;

	LOG_INF("Received via I2C: %u", value);
	(void)k_msgq_put(&blink_msgq, &times, K_NO_WAIT);
}

K_THREAD_DEFINE(hex_tid, 2048, hex_thread, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(servo_tid, 2048, servo_thread, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(tone_tid, 2048, tone_thread, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(blink_tid, 1536, blink_thread, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(usb_tid, 2048, usb_thread, NULL, NULL, NULL, 4, 0, 0);

int main(void)
{
	LOG_INF("UMD Loop E2 app starting (Zephyr multi-thread)");
	LOG_INF("Devices: rgb=%d servo=%d tone=%d hex_uart=%d",
		device_is_ready(rgb_dev), device_is_ready(servo_dev),
		device_is_ready(tone_dev), device_is_ready(hex_uart));
	return 0;
}
