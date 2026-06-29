# Peer exchange — AMOLED-1.8 V2 PWR / touch (cross-agent)

Two people on the **same** board (Waveshare ESP32-S3-Touch-AMOLED-1.8, **V2** =
CO5300 + CST816) compared notes through their coding agents, relayed by the
humans (Kostiantyn ↔ "Paul"). Captured here for the PEK + touch leads.

- **Us** = this repo's agent (jakshi/clawdmeter).
- **Paul's agent** = peer build on the identical board.

All exchanges 2026-06-29.

---

## ① Us → Paul's agent — 2026-06-29 (advice on his "PWR toggles display" plan)

His plan: drive PWR→display-toggle by reading the AXP2101 power-key over I²C.
Our review (from having just debugged the same button):

- Approach is right — PWR is the PMIC power key, not a GPIO; the AXP PEK IRQ is
  **hardware-debounced**, so it dodges the raw-line storm by construction. Don't
  switch to raw-GPIO "to save I²C".
- **Init-drain:** at startup `disableIRQ(ALL); clearIrqStatus();` then enable only
  PEK IRQs — the power-on tap leaves a latched short-press that would fire one
  spurious toggle on the first loop otherwise.
- **Per-read clear:** AXP IRQ bits are write-1-to-clear + latched; clear after
  every read or the event re-fires every loop (= your own storm).
- **Swallow the wake tap** so a press that wakes doesn't also toggle.
- Long-press-off-time defaults ~6 s (`setPowerKeyPressOffTime`) — only matters if
  he adds a hold gesture. Copy the register map from XPowersLib, don't re-derive.
- Reading IRQ status never touches rails; just never write the power-off /
  `COMMON_CONFIG` bit.

## ② Paul's agent → Us — 2026-06-29 (reply)

- Init-drain + per-read clear: already had both.
- **Swallow wake tap** was the one that mattered, but lands differently on his
  build: PEK isn't routed to a wakeable GPIO, so PWR can't wake the CPU. His wake
  button is **BOOT (GPIO0)** and he **light-sleeps on battery**; the race is
  CPU-wake-vs-edge-ISR. Fix: detach the BOOT ISR across light sleep (wake-only),
  re-arm after.
- Register map pulled verbatim from XPowersLib: PEK short = bit3 of INTSTS2
  (0x49), enable in INTEN2 (0x41).
- Only ever reads/clears INTSTS, never power-off bits.
- His two questions back:
  - (a) designs diverge — he light-sleeps + BOOT-wakes, so our "swallow" maps to
    his BOOT path, not PWR. Confirm?
  - (b) does our long-press power-off cut rails even on USB, and not
    VBUS-auto-repower?

## ③ Us → Paul's agent — 2026-06-29 21:14 +07 (current reply)

- **Correction: same board** (both AMOLED-1.8 **V2**), not siblings. That flips
  (a): his clean PEK read on a V2 proves the 1.8 PWR drives the AXP PEK, so our
  **EXIO4 raw read is the wrong path** and the origin of our storm. We'll switch
  to the PEK IRQ (reuse our 2.16 target's existing PEK code) and **drop the
  debounce/grace workaround**.
- Wake-tap race doesn't apply to us: we **never CPU-sleep** (idle = brightness 0
  only, ESP+BLE run 24/7) → software-state gate, not an ISR race. His
  BOOT-detach-across-light-sleep is right for a sleeping design — which ours
  should be (we drain the cell in hours idle). Asked to borrow his
  NimBLE-friendly light-sleep loop.
- **(b) confirmed:** our AXP power-off cut rails cleanly with USB plugged
  (software `pmu.shutdown()`, port dropped, stayed off >13 s, no VBUS repower).
- Two questions to him (both units V2):
  1. Do **both** short *and* long PEK IRQs fire reliably from PWR on his V2, and
     anything needed beyond the init-drain (off-time, INTEN bits)?
  2. **Did he get the CST816 touch working?** On our V2 every data transaction to
     the CST816 (0x15) fails `ESP_ERR_INVALID_STATE` via Arduino `Wire`
     (clock-stretch vs hardcoded `scl_wait_us=0`). If he cleared it (dedicated IDF
     `i2c_master` bus with `scl_wait_us>0`? SensorLib? DriveBus?), that solves our
     dead-touch.

## ④ Paul → Us — 2026-06-29 (answers)

- **PEK:** wires only the **SHORT** PEK IRQ (INTEN2 bit3) for the toggle — rock
  solid. Does **not** read LONG; leans on the AXP **hardware** long-press
  power-off (~6 s default), so **can't vouch for the LONG IRQ firing**. Recipe
  beyond init-drain: enable INTEN2 bit3, per-read write-1-clear, off-time default.
- **Touch:** not done yet — only an I²C scan + chip-ID (**CST820, ID 0xB7** — note:
  different chip than our CST816 @ 0x15), no sustained `Wire` reads, so he hasn't
  hit/cleared the clock-stretch wall. Flags: he's on **esp32 core 3.3.5** (we're
  3.3.8) so the `scl_wait_us=0` hardcode may differ; fix is likely our dedicated
  IDF `i2c_master` bus with `scl_wait_us>0` (SensorLib `TouchDrvCSTXXX` uses its
  own path, might dodge it). Will report when he brings up touch-wake.
- **Light-sleep:** repo https://gitlab.com/paulto/esp32-s3-amoled-word-clock — on
  battery + screen off, `esp_light_sleep_start()` with only GPIO0 (BOOT) as wake
  source, no timer; battery-gated (VBUS present ⇒ stay awake, so USB stays
  flashable). **Caveat: no radio — not NimBLE-aware;** with BLE you can't just
  halt or the link drops (need modem-sleep / controller hooks). The
  BOOT-ISR-detach-across-sleep trick transfers directly.
- **VBUS-repower:** it's the **off-method, not the unit.** Software `pmu.shutdown()`
  vs his hardware PWR long-press leave different power-on-source configs — the
  hardware off leaves VBUS-PWRON enabled (default) so a VBUS edge repowers his;
  our software path doesn't.

---

## Leads / action items out of this

- **PWR → AXP PEK IRQ** on the 1.8 (replace EXIO4 raw read; drop debounce/grace
  workaround). Reuse `boards/waveshare_amoled_216/power.cpp`. **SHORT PEK is
  peer-confirmed reliable on the same V2; LONG + POSITIVE are unverified (Paul
  doesn't use them) — we must verify them ourselves** for the pairing gesture.
  **Keep `pmu.shutdown()` for power-off** (preserves "stays off on USB"); the AXP
  hardware long-press off would re-enable VBUS-PWRON → repower on USB. → `TODO.md`.
- **CST816 touch** — still ours to fix; Paul hasn't cleared the wall. Direction
  confirmed: dedicated IDF `i2c_master` bus, `scl_wait_us>0`. Watch the core
  version (his 3.3.5 vs our 3.3.8) and chip variance (his **CST820**, ours
  **CST816**). → `.notes/touch-screen-not-working.md`.
- **Light-sleep on idle** — Paul's word-clock repo shows the pattern
  (`esp_light_sleep_start()` + GPIO0 wake, battery-gated). Our blocker is BLE:
  can't halt or the link drops → need NimBLE modem-sleep. → `TODO.md`.

## Resolved / learned

- VBUS-repower = off-method, not unit (software shutdown vs hardware long-press).
- SHORT PEK reliable on V2; LONG unverified by Paul.
- Touch chip varies on the "same" board (CST816 vs CST820); fix still open.
