# TODO

## Upper (BOOT) button: Space → Shift+Tab (auto-approvals) 🙂

On the AMOLED-1.8 the top **BOOT** button currently sends **Space** (Claude Code
voice push-to-talk). Change it to **Shift+Tab** so a press cycles Claude Code's
permission mode — i.e. flip into auto-accept / auto-approve.

- **Where:** `firmware/src/main.cpp`, primary-button handler (~line 318):
  currently `ble_keyboard_press(0x2C, 0);  // HID Space, no mods`.
- **Change to:** `ble_keyboard_press(0x2B, 0x02);  // HID Tab (0x2B) + Left Shift (0x02)`.
- **Semantics:** one press = one Shift+Tab (mode cycle). The existing
  press-on-down / release-on-up edges already produce a single tap, so no extra
  logic needed.
- **Gate it per board (don't regress the 2.16):** the 2.16's *left* button is
  Space and its *right* button is already Shift+Tab. Changing the shared
  `primary` mapping globally would make the 2.16 lose voice on its left button.
  Add a `BoardCaps` flag (same pattern as `pwr_toggles_screen`) — e.g.
  `primary_sends_shift_tab` — true on the 1.8, false elsewhere.

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

## Power-off button on the 1.8

Right now the only way to switch the 1.8 off is pulling USB — its PWR button is
on the IO-expander (EXIO4), not the AXP power key, so the 2.16's hardware 8 s
shutdown (`setPowerKeyPressOffTime`) doesn't apply. Add a software off: long-hold
PWR → `pmu.shutdown()` (AXP2101, confirmed in XPowersLib at `XPowersAXP2101.tpp`).

- **Deconflict gestures:** 3 s-hold + release already = clear BLE bond (pairing).
  Use a longer hold (~6 s) or a distinct gesture for power-off so they don't
  collide. The PWR hold/pair state machine is in `main.cpp` (~line 248); `pmu`
  lives in `boards/waveshare_amoled_18/power.cpp`.
- **Gate per board:** the 2.16 already powers off via AXP hardware; only the 1.8
  needs this software path.

## Touch (CST816 / V2 panel)

Dead — see [`.notes/touch-screen-not-working.md`](.notes/touch-screen-not-working.md).
Stopgap shipped: PWR toggles splash↔usage (`feature/pwr-toggle-screens`).

- [ ] **File an upstream issue** on `HermannBjorgvin/Clawdmeter`: V2 (CST816)
  touch is dead — every data I2C transaction fails `ESP_ERR_INVALID_STATE` via
  Arduino `Wire` on arduino-esp32 3.3.8 (address probe ACKs, so `board_rev`
  detects V2 and the display works; reads/data-writes don't). Summarize the root
  cause + ruled-out theories from `.notes/touch-screen-not-working.md`; flag that
  the FT3168 rev is likely the only one tested. Pair with a PR once the fix lands.
