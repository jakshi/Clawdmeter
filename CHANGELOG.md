# Changelog

Changes in this fork (`jakshi/Clawdmeter`). Upstream: `HermannBjorgvin/Clawdmeter`.

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
  `BoardCaps.pwr_software_shutdown`. The 1.8's PWR button is on the IO
  expander, not the AXP power key, so the PMU's own long-press shutdown never
  fires. Effective on battery; on USB the VBUS re-powers.

## 2026-06-27

### Added
- **AMOLED-1.8: PWR button toggles splash ↔ usage**, making the usage screen
  reachable without touch. The V2 (CST816) panel's touch is dead (see
  [`.notes/touch-screen-not-working.md`](.notes/touch-screen-not-working.md));
  this is the stopgap. Gated by `BoardCaps.pwr_toggles_screen`.
