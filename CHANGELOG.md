# Changelog

Changes in this fork (`jakshi/Clawdmeter`). Upstream: `HermannBjorgvin/Clawdmeter`.

## 2026-06-29

### Fixed
- **AMOLED-1.8: spurious screen-toggle storm on wake from power-off.** A cold
  (AXP power-on) boot let the EXIO4 PWR line emit ~11 phantom short-press edges
  in the first ~0.5 s, rapidly flipping splash↔usage until it settled. Added a
  debounce (3 consecutive samples) + boot-grace (ignore PWR for the first ~2 s)
  + state seeding in `boards/waveshare_amoled_18/power.cpp`. Diagnosed with an
  NVS flight-recorder + trigger counters, since removed — restore steps in
  [`.notes/doc/diagnostic-scaffolding.md`](.notes/doc/diagnostic-scaffolding.md).

### Notes
- Correction to the 2026-06-28 power-off entry: the 1.8's PWR button is wired to
  **both** EXIO4 (firmware-read) **and** the AXP PWRKEY (a tap wakes it from a
  full shutdown with no USB). `pmu.shutdown()` powers off cleanly on battery
  **and on USB** — it does not auto-re-power from VBUS.
- Added [`.notes/doc/`](.notes/doc/): Waveshare/Espressif datasheet + reference-repo
  links and findings (official firmware uses no software PWR button / no software
  power-off; CST816 needs `0xFA`/`0xE5` init — candidate dead-touch fix).

## 2026-06-28

### Added
- **Battery: numeric NN% readout** replaces the 5-state `[|||]` icon. Drawn as a
  battery shell (rounded-rect outline + terminal nub) with the percentage
  centered inside, in the mascot terra-cotta (`COL_ACCENT`); turns green while
  charging. Hidden when the board reports no battery (`pct < 0`). New
  compile-time flag `BATTERY_SHOW_PERCENT` (default `1`); a board opts back to
  the old icon with `-D BATTERY_SHOW_PERCENT=0` in its `build_flags`.
- **AMOLED-1.8: BOOT button sends Shift+Tab** (Claude Code mode cycle /
  auto-accept) instead of Space. Gated by the new
  `BoardCaps.primary_sends_shift_tab`; other boards keep Space.
- **AMOLED-1.8: long-hold PWR (~6 s) powers the device off** via the new
  `power_hal_shutdown()` → AXP2101 `pmu.shutdown()`. Gated by
  `BoardCaps.pwr_software_shutdown`. The 1.8's PWR button is read via the IO
  expander (EXIO4), so the firmware synthesizes the long-press. (Corrected
  2026-06-29: PWR is *also* wired to the AXP PWRKEY, and `pmu.shutdown()` powers
  off cleanly on battery **and** on USB — it does not auto-re-power from VBUS.)

## 2026-06-27

### Added
- **AMOLED-1.8: PWR button toggles splash ↔ usage**, making the usage screen
  reachable without touch. The V2 (CST816) panel's touch is dead (see
  [`.notes/touch-screen-not-working.md`](.notes/touch-screen-not-working.md));
  this is the stopgap. Gated by `BoardCaps.pwr_toggles_screen`.
