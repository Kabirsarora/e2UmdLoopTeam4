# Copyright (c) 2026 UMD Loop
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_UMD_LOOP_RGB_LED rgb_led)
add_subdirectory_ifdef(CONFIG_UMD_LOOP_SERVO servo)
add_subdirectory_ifdef(CONFIG_UMD_LOOP_TONE tone)
add_subdirectory_ifdef(CONFIG_UMD_LOOP_COMMS comms)
