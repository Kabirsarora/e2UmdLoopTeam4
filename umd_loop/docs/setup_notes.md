# Local toolchain (ESP32 build + flash)

## Installed on this PC

| Component | Location / version |
|-----------|-------------------|
| Python 3.12 venv + west | `%USERPROFILE%\zephyrproject\.venv` (west 1.5.0) |
| Zephyr source + modules | `%USERPROFILE%\zephyrproject\` (`west update` done) |
| CMake, Ninja, gperf, dtc, 7-Zip | winget (add **7-Zip** to PATH for SDK setup) |
| VS 2022 Build Tools (MSVC) | Host compiler for CMake |
| Zephyr SDK **1.0.1** | `%USERPROFILE%\zephyr-sdk-1.0.1` |
| Xtensa ESP32 toolchain | `xtensa-espressif_esp32_zephyr-elf` |
| Espressif HAL blobs | `west blobs fetch hal_espressif` |
| esptool | v5.4+ via `west packages pip --install` |

## Every new PowerShell session (before build/flash)

```powershell
& $HOME\zephyrproject\.venv\Scripts\Activate.ps1

$env:Path = @(
  "$HOME\zephyrproject\.venv\Scripts",
  "C:\Program Files\CMake\bin",
  "C:\Program Files\7-Zip",
  "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe",
  "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\oss-winget.dtc_Microsoft.Winget.Source_8wekyb3d8bbwe",
  $env:Path
) -join ';'

$env:ZEPHYR_BASE = "$HOME\zephyrproject\zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = "$HOME\zephyr-sdk-1.0.1"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
```

For MSVC, open **“x64 Native Tools Command Prompt for VS 2022”** or run `vcvars64.bat` before `west build` if the compiler is not found.

## Build UMD Loop app

Board target (Zephyr 4.4): **`esp32_devkitc/esp32/procpu`** (not `esp32_devkitc_wroom`).

Overlay file: `app/boards/esp32_devkitc_esp32_procpu.overlay`

```powershell
cd <repo>\umd_loop\app
west build -p always -b esp32_devkitc/esp32/procpu
```

Verified: build completes and produces `build/zephyr/zephyr.bin`.

## Flash (board on USB)

```powershell
west flash
# or monitor after flash:
west espressif monitor
```

Use **115200** on the ESP32 USB port for `H` / `S` / `T` / `N` commands.

## If SDK setup fails again

Ensure `7z` is on PATH (`C:\Program Files\7-Zip`), then:

```powershell
cd $HOME\zephyr-sdk-1.0.1
cmd /c "setup.cmd /t xtensa-espressif_esp32_zephyr-elf /h"
```
