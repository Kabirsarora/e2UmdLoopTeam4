# Local toolchain notes (code-only)

Installed for E2 authoring (no ESP32 SDK / no flash):

| Component | Location |
|-----------|----------|
| Python 3.12 venv | `%USERPROFILE%\zephyrproject\.venv` |
| `west` | `%USERPROFILE%\zephyrproject\.venv\Scripts\west.exe` |
| Zephyr tree | `%USERPROFILE%\zephyrproject\` (after `west init` / `west update`) |

Activate:

```powershell
& $HOME\zephyrproject\.venv\Scripts\Activate.ps1
```

ESP32 Zephyr SDK, Espressif blobs, MSVC Build Tools, and `west flash` were
intentionally **not** installed (code-only workflow).
