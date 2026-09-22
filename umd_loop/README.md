# UMD Loop — Zephyr E2 Module

Out-of-tree Zephyr module that ports the E1 Arduino systems into reusable
device drivers, plus a multi-threaded application that exercises them together.

## Layout

```
umd_loop/
├── CMakeLists.txt / Kconfig / zephyr/module.yml
├── app/                 # west build application
├── drivers/             # rgb_led, servo, tone, comms
├── include/umd_loop/    # public APIs
├── dts/bindings/        # DT bindings
├── boards/              # esp32_devkitc_wroom.overlay
└── docs/                # Part 0–2 presentation notes
```

## Build (when Zephyr SDK is installed)

From a Zephyr west workspace:

```powershell
$env:ZEPHYR_BASE = "$HOME\zephyrproject\zephyr"
cd <repo>\umd_loop\app
west build -p always -b esp32_devkitc_wroom -- `
  -DEXTRA_ZEPHYR_MODULES=<repo>/umd_loop `
  -DDTC_OVERLAY_FILE=<repo>/umd_loop/boards/esp32_devkitc_wroom.overlay
```

Or set the overlay via `app/boards/esp32_devkitc_wroom.overlay` symlink/copy.

## USB console commands (115200)

| Command | Action |
|---------|--------|
| `H#FF0000` | Nearest primary RGB LED (also bare `#FF0000`) |
| `S90` | Servo to 90°, home after 2 s |
| `T440` | Tone at 440 Hz; duration from ADC on GPIO34 |
| `N123` | Blink status LED 5× |

Wire HEX on UART1 (GPIO16 RX) at 9600 also feeds the RGB path.
