# Touch screen not working — AMOLED-1.8 V2 (CST816)

**Status:** unresolved (root cause found, fix deferred). Diagnosed 2026-06-27.
**Affected:** Waveshare ESP32-S3-Touch-AMOLED-1.8, **V2 panel** = CO5300 display
+ **CST816 touch @ I2C 0x15**. (Original rev = SH8601 + FT3168 @ 0x38 — untested
here, may or may not share the bug.)
**Toolchain:** arduino-esp32 **3.3.8** (pioarduino), new IDF `i2c_master` driver
("i2c-ng"). Env `waveshare_amoled_18`.

## Symptom

On-device touch does nothing — taps never switch screens. Display, buttons,
BLE, daemon all fine. The splash↔usage tap toggle (`global_click_cb` in
`ui.cpp`) never fires because the touch read never reports a press.

## Root cause (confirmed)

**Every *data* I2C transaction to the CST816 (0x15) via Arduino `Wire` fails with
`ESP_ERR_INVALID_STATE` (259)** — from the very first attempt at boot:

```
[E][esp32-hal-i2c-ng.c:372] i2cWriteReadNonStop(): i2c_master_transmit_receive failed: [259] ESP_ERR_INVALID_STATE
[E][Wire.cpp:520] requestFrom(): i2cWriteReadNonStop returned Error 259
Touch ID read failed (addr 0x15)
```

- The **address-only probe ACKs** (`Wire.beginTransmission(0x15); Wire.endTransmission();`
  with no data byte → returns 0), which is why `board_rev()` correctly detects V2
  and the display works.
- But any transaction that moves **data** — `requestFrom` (read) or a register
  write with a data byte — fails `INVALID_STATE`.
- **Every other device on the same bus works**: XCA9554 IO-expander (0x20, read
  every loop for the PWR button), AXP2101 PMU (0x34), QMI8658 IMU (0x6B). Bus
  pins: `SDA=15, SCL=14`.

So it is specific to the CST816 device + Arduino `Wire`, not the bus.

## Ruled out (do NOT re-test these)

1. **Auto-sleep.** CST816 standbys ~2 s after touch, but the read fails at boot
   (t≈1.5 s), before any sleep. Writing reg `0xFE = 0xFE` (DisAutoSleep, per
   fbiego/CST816S) made no difference.
2. **I2C clock speed.** XPowersLib (`XPOWERSLIB_I2C_MASTER_SPEED 400000`) and
   SensorLib (`SENSORLIB_I2C_MASTER_SEEED 400000`) raise the shared bus to
   400 kHz before touch init. Forcing `Wire.setClock(100000)` before the touch
   read did **not** fix it — fails at both 100 kHz and 400 kHz.
3. **STOP vs repeated-start.** `endTransmission(false)` (combined write-read) →
   the *read* fails. `endTransmission(true)` (separate write+read) → the *write*
   fails (`et=4`). Both `INVALID_STATE`.
4. **TP_INT not firing.** Real and separate: with the original INT-gated read,
   zero reads occur (TP_INT never fires on this CST816). But polling instead
   still hits the I2C failure — so INT is secondary, not the blocker.
5. **`scl_wait_us` (clock-stretch tolerance).** Leading mechanism, but **not
   fixable through `Wire`:** arduino-esp32's `i2cAddDeviceIfNeeded`
   (`esp32-hal-i2c-ng.c`) hardcodes `dev_config.scl_wait_us = 0` and there is no
   `Wire` API to change it. SensorLib also sets 0. The CST816 appears to hold /
   clock-stretch the bus in a way the IDF `i2c_master` driver rejects with
   `INVALID_STATE`; the other chips don't stretch, so they're fine.

## Why this isn't a one-line fix

- `Wire` (i2c-ng) gives no knob for `scl_wait_us` and **exposes no bus-handle
  getter** (`i2cGetBusHandle` does not exist in 3.3.8), so you can't grab Wire's
  IDF bus and add the CST816 with a tuned device config.
- This is almost certainly why upstream's V2 touch never worked — the author's
  board was the FT3168 revision, so the CST816 path looks untested.

## Fix direction (for the dedicated session)

1. **Try SensorLib `TouchDrvCSTXXX`** (already a linked dependency,
   `firmware/.pio/libdeps/.../SensorLib/src/TouchDrvCSTXXX.hpp`). It has a real
   reset sequence and an IDF-native i2c path. Quickest to try; may still hit the
   Wire wall if pointed at `Wire`.
2. **Dedicated IDF `i2c_master` bus for the CST816.** ESP32-S3 has 2 I2C ports;
   `Wire` owns port 0. Create an `i2c_master` bus on **port 1**, same pins
   (15/14), `i2c_master_bus_add_device` with `scl_wait_us > 0` (clock-stretch
   tolerance), then `i2c_master_transmit_receive`. Two controllers on shared
   pins is fine cooperatively (single-threaded, blocking transactions). Keep the
   FT3168 path on `Wire`.
3. Report upstream (`HermannBjorgvin/Clawdmeter`) as a V2-panel touch bug.

## Stopgap shipped

PWR button toggles splash↔usage so the usage screen is reachable without touch.
Branch `feature/pwr-toggle-screens` on the `jakshi/Clawdmeter` fork; gated by a
new `BoardCaps.pwr_toggles_screen` flag (1.8 only, no 2.16 regression).

## Debug tooling that worked

- Add throttled `Serial.printf("[TP] ...")` inside `touch_read_into_shared_state`
  (`firmware/src/boards/waveshare_amoled_18/touch.cpp`) to log `et` / `got` /
  finger count.
- Capture serial with a **pyserial** reader at 115200 — `pio device monitor`
  fails when backgrounded (`termios.error: Operation not supported by device`,
  needs a TTY). pyserial lives at `~/.platformio/penv/bin/python`.
- For the **boot log**, toggle RTS to reset first (esptool resets "via RTS pin"
  on this board): `ser.setRTS(True); sleep(0.15); ser.setRTS(False)`.

## Key references

- `esp32-hal-i2c-ng.c` — `i2cAddDeviceIfNeeded` (line ~214) hardcodes
  `scl_wait_us = 0`; `i2cWrite` / `i2cWriteReadNonStop` are where 259 surfaces.
- fbiego/CST816S — disable auto-sleep = write `0xFE` to reg `0xFE`.
- Touch data layout (both panels): reg `0x02` low nibble = finger count,
  `0x03/0x04` = X hi/lo, `0x05/0x06` = Y hi/lo.
