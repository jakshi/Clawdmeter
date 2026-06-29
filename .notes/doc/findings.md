# Findings — references vs. our firmware (2026-06-28)

What the official Waveshare code/datasheets told us, and how our firmware
differs. Driven by debugging the AMOLED-1.8 V2 "wake-up after power-off" issue.

## PWR button & power-off

**Official:** `pin_config.h` defines **no PWR-button GPIO**. The bring-up sets
XCA9554 **EXIO0,1,2,6 as outputs** (LCD/TP resets + audio amp enable) and never
reads EXIO4. Power on/off is handled **entirely by the AXP2101 PWRKEY in
hardware**. No software shutdown.

**Ours:** reads PWR via **EXIO4** (active HIGH) and does a software
`pmu.shutdown()` on a ~6 s hold (`pwr_software_shutdown` cap). This was added on
the assumption "PWR isn't on the AXP power key" — but a tap wakes the device
from a full shutdown with no USB, which is only possible via the **AXP PWRKEY**.
So PWR is wired to **both** PWRKEY (hardware on/off) and EXIO4 (firmware read).

**Confirmed this session (NVS flight-recorder + trigger counters):**
- The "screen storm on wake" = **EXIO4 fires ~11 phantom short-press edges in
  the first ~0.5 s of a cold (AXP power-on) boot** → 11 spurious splash↔usage
  toggles. Touch generated **0** events — not touch-related.
- **Fix:** debounce (2 equal samples) + boot-grace (drain PWR events for the
  first ~1.2 s) + seed `last_pwr_state` in `boards/waveshare_amoled_18/power.cpp`.
  Storm 11 → 2 (residual benign/even). A plain ESP reset does NOT reproduce it;
  only a real AXP power-on does (rails cycle).
- `pmu.shutdown()` **cleanly powers off even on USB and does NOT auto-reboot
  via VBUS.** → the CLAUDE.md note "on USB the VBUS re-powers" is **wrong for
  V2**; correct it.
- Power-off genuinely works: a 20 h off was truly off (screen dark, zero uptime
  accrued). Battery stayed healthy throughout (`minbootmv` ≈ 4044 mV); the short
  cold-boot cycles never browned out. The one rare 20 h-off brownout is still
  unexplained but is **not** a flat cell.

### PWR read path: EXIO4 (ours) vs AXP PEK (better) — peer-confirmed (2026-06-29)

Cross-checked with a peer running the **same** AMOLED-1.8: they read PWR via the
**AXP2101 PEK IRQ** (hardware-debounced) and never hit a storm. That confirms the
1.8's PWR drives the AXP PEK, so our **EXIO4 raw read is the inferior path** and
the real origin of the cold-boot storm; the debounce/grace fix is a workaround.
Planned: switch to the PEK path (mirror `waveshare_amoled_216/power.cpp`) — see
TODO. Their init-drain (`disableIRQ(ALL); clearIrqStatus()` before enabling PEK)
is the clean analog of our boot-grace: it discards the power-on tap's latched
short-press. Both units are **V2 (CO5300/CST816)**; open question to the peer is
just whether both short + long PEK fire reliably from PWR on their unit.

Also surfaced: they **light-sleep + BOOT-wake**; we never CPU-sleep (idle = dim
only) → our cell drains in hours idle. Worth adopting their light-sleep approach.

## CST816 touch ("dead touch", separate issue)

**The root cause is NOT missing register init** — see
[`../touch-screen-not-working.md`](../touch-screen-not-working.md). Every *data*
I2C transaction to the CST816 fails `ESP_ERR_INVALID_STATE`: the chip
clock-stretches and arduino-esp32 3.3.8's `Wire` (i2c-ng) hardcodes
`scl_wait_us = 0` with no API to raise it. So register **writes themselves fail**
— `0xFA`/`0xE5` can't even be applied — and `0xE5` auto-sleep ≈ the `0xFE`
DisAutoSleep already tried with no effect. (The official firmware sidesteps this
by talking to the CST816 through `Arduino_DriveBus`'s own IDF I2C path, not
shared `Wire`.)

**Fix direction (per the notes):** a dedicated IDF `i2c_master` bus on **port 1**
with `scl_wait_us > 0`, or SensorLib `TouchDrvCSTXXX` — not register init.
CST816D regs for reference: `0x02` finger count, `0x03-0x06` X/Y, `0xE5` sleep,
`0xFA` interrupt mode, `0xA7` ID.

## AXP2101

**Official** example does **not** configure the AXP at all (runs on power-on
defaults). **Ours** sets `setSysPowerDownVoltage(2600)`; the 2.16 also sets
`setPowerKeyPressOffTime(8S)` but the 1.8 sets no PWRKEY timing.

## Version divergence (ours vs official)

| | Official | Ours |
|--|----------|------|
| LVGL | 8.4.0 | 9.5 |
| Core | esp32 ≥3.0.6 | pioarduino platform-espressif32 |
| GFX | 1.4.9 | 1.6.6 |
| Touch | Arduino_DriveBus (GPLv3) | custom inline reader |
| PWR | AXP PWRKEY hardware | EXIO4 read + software shutdown |
