# Part 1 — Programming each system (Arduino → Zephyr)

## Process overview

1. Capture E1 behavior from the Arduino sketches in the repo root.
2. Split **hardware access** into Zephyr device drivers (`umd_loop/drivers/*`).
3. Split **scheduling / protocol** into RTOS threads + `k_msgq` in `app/src/main.c`.
4. Describe wiring once in Devicetree (`boards/esp32_devkitc_wroom.overlay`).

## Per-system notes

### System 1 — HEX UART → RGB (`hexanypin.ino`)

| Arduino | Zephyr |
|---------|--------|
| `digitalWrite` on 25/26/27 | `rgb_led` driver + `gpio_dt_spec` |
| `Serial2` @ 9600 after pin detect | UART1 fixed RX=16 in overlay |
| `parseHex` / `dist2` in sketch | `comms_parse_hex` + `rgb_led_nearest_primary` |
| Busy `loop()` polling | `hex_thread` + message queue |

### System 2 — Servo (`E1_System_2`)

| Arduino | Zephyr |
|---------|--------|
| `ESP32Servo` / `write(angle)` | `servo` PWM driver (50 Hz, 500–2000 µs) |
| `Serial.parseInt` + `delay(2000)` | USB `S` command → `servo_thread` sleeps with `k_msleep` |

### System 3 — Tone + ADC (`E1 System 3`)

| Arduino | Zephyr |
|---------|--------|
| `tone(pin, freq, duration)` | PWM square wave via `tone_play` |
| `analogRead` + `map` | `adc_read_dt` + duration map in driver |

### I2C / USB blink (`I2C_receiver`, `USB_comm`)

| Arduino | Zephyr |
|---------|--------|
| `Wire.onReceive` + blink in `loop` | `umd_loop_i2c_byte_received` → `blink_msgq` |
| `Serial.parseInt` 0–255 → 5 blinks | USB `N` command → same blink thread |

## Arduino IDE limitations (ESP32)

Recognized limits that motivated the Zephyr port:

1. **Cooperative single `loop()`** — no preemptive threads; long `delay()` in
   servo/tone starves UART HEX reception.
2. **Weak modularity** — pins, protocol, and actuators live in one `.ino`; hard
   to reuse across boards or teammates.
3. **No Devicetree** — pin changes require editing source; Zephyr moves that to
   overlay/bindings.
4. **Timing jitter** — SoftwareSerial (Uno) and blocking Arduino APIs make
   multi-task deadlines soft and unpredictable.
5. **Library fragmentation** — `ESP32Servo` / `tone()` are core-specific; Zephyr
   PWM/ADC APIs are portable across SoCs with DT.
6. **Limited concurrency primitives** — no first-class `k_msgq` / mutex story
   without FreeRTOS calls mixed into sketch code.

Zephyr addresses these with driver modules, DT instantiation, and one thread
per subsystem coordinated by message queues.
