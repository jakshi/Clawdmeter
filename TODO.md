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

## Rewrite the daemon/server in Go

Current host daemon is a Bash script (`daemon/claude-usage-daemon.sh`) plus a
Python BLE bridge. Rewrite as a single static Go binary.

- Replace bash + python with one cross-platform Go program (BLE via `tinygo-org/bluetooth`
  or similar; HTTP via stdlib).
- Keep behavior: read OAuth token (macOS Keychain `Claude Code-credentials` /
  Linux file), poll Anthropic usage API, push JSON over BLE GATT
  (`4c41555a-...0001` RX `...0002`), honor the firmware REQ-refresh notify on `...0004`.
- Single binary simplifies install (no venv, no dbus-monitor pipe). Revisit the
  Homebrew-formula item — Go binary is `brew` bottle-friendly.

## Touch (CST816 / V2 panel)

Dead — see [`.notes/touch-screen-not-working.md`](.notes/touch-screen-not-working.md).
Stopgap (PWR toggles splash↔usage) already shipped — see CHANGELOG (2026-06-27).

- [ ] **File an upstream issue** on `HermannBjorgvin/Clawdmeter`: V2 (CST816)
  touch is dead — every data I2C transaction fails `ESP_ERR_INVALID_STATE` via
  Arduino `Wire` on arduino-esp32 3.3.8 (address probe ACKs, so `board_rev`
  detects V2 and the display works; reads/data-writes don't). Summarize the root
  cause + ruled-out theories from `.notes/touch-screen-not-working.md`; flag that
  the FT3168 rev is likely the only one tested. Pair with a PR once the fix lands.
