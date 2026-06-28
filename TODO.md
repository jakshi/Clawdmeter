# TODO

## Package the macOS daemon as a Homebrew formula

Current setup is sloppy — `install-mac.sh` hand-rolls a venv in `daemon/.venv/`
and renders/loads a LaunchAgent plist. Replace with a formula in the personal
tap (`jakshi/homebrew-tap`) so it's `brew install jakshi/tap/clawdmeter` +
`brew services start clawdmeter`.

- Formula outline: `depends_on "python@3.x"`; declare `bleak` + `httpx` as
  `resource`s and `virtualenv_install_with_resources`; install
  `daemon/claude_usage_daemon.py`; add a `service do … end` block (launchd) so
  `brew services` manages start/stop/login.
- Keep reading the OAuth token from the macOS Keychain (service
  `Claude Code-credentials`).
- Retire `install-mac.sh` + the manual LaunchAgent once the formula works
  (or have the script just shell out to brew).
- Linux/Windows installers stay as-is.

## Battery: show percentage instead of the icon

Replace the 5-state battery icon (`[|||]`) with a numeric **NN%** readout.

- **Where:** `ui_update_battery(pct, charging)` in `ui.cpp` currently swaps the
  `battery_img` (RGB565A8 icons from `icons.h`). Change it to drive an
  `lv_label` showing the percent instead (or icon + "%").
- **Data's already there:** `power_hal_battery_pct()` returns 0..100 (or -1 when
  `!BoardCaps.has_battery` — hide the label in that case).
- Pick a small Styrene/Mono font for the slot; show a charging glyph or "⚡"
  when `charging`.
- Optional: keep it behind a flag if you want the icon on some boards and the
  number on others.

## Touch (CST816 / V2 panel)

Dead — see [`.notes/touch-screen-not-working.md`](.notes/touch-screen-not-working.md).
Stopgap (PWR toggles splash↔usage) already shipped — see CHANGELOG (2026-06-27).

- [ ] **File an upstream issue** on `HermannBjorgvin/Clawdmeter`: V2 (CST816)
  touch is dead — every data I2C transaction fails `ESP_ERR_INVALID_STATE` via
  Arduino `Wire` on arduino-esp32 3.3.8 (address probe ACKs, so `board_rev`
  detects V2 and the display works; reads/data-writes don't). Summarize the root
  cause + ruled-out theories from `.notes/touch-screen-not-working.md`; flag that
  the FT3168 rev is likely the only one tested. Pair with a PR once the fix lands.
