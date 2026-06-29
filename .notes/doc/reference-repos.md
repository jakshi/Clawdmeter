# Reference repositories

## Official Waveshare example (authoritative)

- URL: https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8
- Clone (read-only reference; NOT vendored into this repo):
  ```bash
  git clone --depth 1 https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8
  ```
- During the 2026-06-28 session it was cloned to a throwaway scratchpad
  (`.../scratchpad/ws18`) which is session-temporary and now gone — re-clone
  with the command above if needed.
- Key paths:
  - `examples/Arduino-v3.3.5-v2/` — the **V2 panel** variant (CO5300 + CST816), matches our board rev.
  - `examples/Arduino-v3.3.5-v2/libraries/Mylibrary/pin_config.h` — official pin map.
  - `examples/Arduino-v3.3.5/libraries/Arduino_DriveBus/src/touch_chip/Arduino_CST816x.{h,cpp}` — CST816 driver (GPLv3).
  - `examples/.../13_LVGL_Widgets/13_LVGL_Widgets.ino` — full board bring-up (XCA9554, QMI, CST816, GFX).

Official library versions (from the Arduino setup page): GFX_Library_for_Arduino
v1.4.9, **LVGL v8.4.0**, SensorLib v0.2.1, XPowersLib v0.2.6, Arduino_DriveBus,
ESP32_IO_Expander v0.0.3, esp32 core ≥3.0.6. (We diverge: LVGL 9.5, pioarduino
core; see `findings.md`.)

## Community Showcase (inspiration)

From the Waveshare Resources page's Community Showcase:

- **Claude Desktop Buddy** — Claude-themed buddy on this exact board (most relevant):
  https://github.com/vthinkxie/claude-desktop-buddy-esp32
- Step Counter / Pedometer: https://github.com/VolosR/stepCounter
- PocketClock: https://github.com/VolosR/pocketClock
- Stopwatch UI: https://github.com/VolosR/stopwatchAmoled
- Audio Visualizer (spectrum): https://github.com/VolosR/spectrum24
- Smartwatch UI (SquareLine): https://github.com/nishad2m8/Squareline-OBP
- APP PIXELS: https://github.com/app-pixels
