# Part 0 — Schematic revisions, power, programming, BOM

## Why revise the E1 schematic?

Each E1 Arduino sketch assumed it owned the ESP32 alone. Running **all**
systems on one MCU creates pin collisions:

| GPIO | E1 conflict |
|------|-------------|
| 26 | System 1 `LED_G` **and** System 3 buzzer |
| 13 | System 2 servo **and** System 1 UART RX candidate |
| 2 | Status LED (I2C_receiver / USB_comm); also ESP32 strap pin |

## Consolidated pin map (E2 overlay)

| Function | GPIO / bus | Notes |
|----------|------------|-------|
| RGB R / G / B | 25 / **14** / 27 | Green moved from 26 → 14 |
| Servo PWM | 13 (LEDC CH0) | 50 Hz, 500–2000 µs |
| Buzzer PWM | 26 (LEDC CH1) | Square-wave tone |
| ADC duration | 34 (ADC1 CH6) | Map 0–4095 → 200–2000 ms |
| HEX UART RX / TX | 16 / 33 | Fixed RX (no auto-detect); 9600 8N1 |
| USB console | UART0 | 115200 |
| Status LED | 2 | Keep boot/strapping caution |
| I2C SDA / SCL | 21 / 22 | Slave address **8** |

## Safe power

1. **ESP32 DevKitC** powered from USB 5 V (on-board regulator → 3.3 V logic).
2. **Common GND** between Uno, ESP32, servo, and sensor.
3. **Uno → ESP32 HEX wire:** level shift with resistor divider  
   `Uno TX --[1 kΩ]--+-- ESP32 RX`  
   `               [2 kΩ]`  
   `                GND`  
   (Uno is 5 V; ESP32 GPIOs are 3.3 V tolerant only with this divider.)
4. **Servo:** prefer a **separate 5 V supply** (or DevKit 5 V pin only if current
   is small). Never power a large servo from the 3.3 V rail.
5. **RGB LEDs:** series resistors (~220 Ω) from GPIO to LED anode (active-high).

## Programming the circuit

| Path | How |
|------|-----|
| **Zephyr (E2)** | USB data cable → `west flash` / esptool; hold BOOT if needed |
| **Arduino E1 (reference)** | Arduino IDE + ESP32 board package; same USB UART |
| **Uno HEX sender** | Arduino IDE as AVR Uno |

E2 development in this repo is **code-first**; flashing is optional for grading
demos but documented here for Part 0 completeness.

## Bill of Materials

| Qty | Item | Purpose |
|-----|------|---------|
| 1 | ESP32-WROOM DevKitC | Main MCU (Zephyr target) |
| 1 | Arduino Uno (or Nano) | HEX wire sender @ 9600 |
| 1 | Hobby servo (5 V) | System 2 |
| 1 | Passive piezo / magnetic buzzer | System 3 |
| 1 | Potentiometer or analog sensor | ADC on GPIO34 |
| 3 | LEDs (R/G/B) + 3× ~220 Ω | System 1 |
| 1 | LED + resistor | Status (GPIO2) — or use on-board LED |
| 1 | 1 kΩ + 1× 2 kΩ | 5 V→3.3 V divider |
| 1 | Breadboard + jumpers | Wiring |
| 1 | USB-A/C cables (×2) | ESP32 + Uno programming/power |

Optional: external 5 V wall adapter for servo if stall current is high.
