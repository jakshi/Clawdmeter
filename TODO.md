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

## AMOLED-1.8: move PWR from the EXIO4 raw read to the AXP PEK IRQ

The 1.8 currently reads the PWR button as a raw line on XCA9554 **EXIO4**. That
line spits ~11 phantom edges in the first ~0.5 s of a cold boot → the
screen-toggle storm (now masked by a debounce + boot-grace workaround in
`boards/waveshare_amoled_18/power.cpp`). Root cause: PWR actually drives the
**AXP2101 PEK** (a tap wakes the board from a full shutdown — only the PWRKEY
can do that), and the PEK IRQ is **hardware-debounced**. The original EXIO4
choice came from the now-corrected assumption "PWR isn't on the AXP power key."

- **Evidence it works on this board:** a peer's agent runs the *same* AMOLED-1.8
  reading PWR via the AXP PEK IRQ cleanly. Our own `waveshare_amoled_216`
  target already implements the PEK path — copy it.
- **Plan:** mirror `boards/waveshare_amoled_216/power.cpp` — at init
  `disableIRQ(ALL); clearIrqStatus(); enableIRQ(PKEY_SHORT|PKEY_LONG|PKEY_POSITIVE);`
  (the init-drain kills the boot-tap's latched short-press), then per loop
  `getIrqStatus(); isPekeyShort/Long/PositiveIrq(); clearIrqStatus();`. CPU never
  sleeps here, so polling the PEK is fine.
- **De-risk before cutover:** add the PEK read *alongside* the EXIO4 path on our
  unit and log which fires. A peer on the same V2 confirms **SHORT** PEK is
  rock-solid; **LONG + POSITIVE are unverified** (he doesn't use them) — we need
  LONG/POSITIVE for the pairing gesture, so verify them ourselves. **Keep
  `pmu.shutdown()` for power-off** — leaning on the AXP hardware long-press off
  re-enables VBUS-PWRON → the device would repower on USB (the VBUS-repower
  difference is off-method, not unit). Only then drop the EXIO4 read +
  debounce/grace workaround.
## AMOLED-1.8: real light-sleep on idle (battery life)

We never sleep — idle just dims to brightness 0; the ESP32-S3 + BLE run 24/7, so
the cell drains in hours. Peer's pattern
(https://gitlab.com/paulto/esp32-s3-amoled-word-clock): `esp_light_sleep_start()`
on battery + screen off, **only GPIO0 (BOOT) as wake source**, no timer;
battery-gated (VBUS present ⇒ stay awake, so USB stays responsive/flashable).

- **Blocker for us: BLE.** Can't just halt the CPU or the daemon link drops —
  needs NimBLE modem-sleep / the controller's own light-sleep hooks (peer runs no
  radio, so his loop isn't BLE-aware). The **BOOT-ISR-detach-across-sleep** trick
  (wake press = wake-only) transfers directly. See `.notes/doc/peer-exchange.md`.

## Touch (CST816 / V2 panel)

Dead — see [`.notes/touch-screen-not-working.md`](.notes/touch-screen-not-working.md).
Stopgap (PWR toggles splash↔usage) already shipped — see CHANGELOG (2026-06-27).

- [ ] **File an upstream issue** on `HermannBjorgvin/Clawdmeter`: V2 (CST816)
  touch is dead — every data I2C transaction fails `ESP_ERR_INVALID_STATE` via
  Arduino `Wire` on arduino-esp32 3.3.8 (address probe ACKs, so `board_rev`
  detects V2 and the display works; reads/data-writes don't). Summarize the root
  cause + ruled-out theories from `.notes/touch-screen-not-working.md`; flag that
  the FT3168 rev is likely the only one tested. Pair with a PR once the fix lands.
