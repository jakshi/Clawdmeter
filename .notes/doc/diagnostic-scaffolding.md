# Diagnostic scaffolding (removed — restore if the 20 h brownout recurs)

This temporary instrumentation was used to diagnose the AMOLED-1.8 V2
"wake-after-power-off" issue (2026-06-28). It was removed from the tree after
the screen-storm root cause was found and fixed (PWR debounce + boot-grace in
`power.cpp`). Re-apply the blocks below if the **rare 20 h-off brownout** needs
further chasing. All blocks were marked `// TEMP DIAG`.

## Serial commands it added (115200 baud)

- `pmu` — dump NVS flight-recorder + trigger counters + live AXP rail/charger state.
- `off` — call `power_hal_shutdown()` (test power-off; on USB it stays off).
- `diagreset` — clear the NVS recorder (`diag` namespace).

### How to read it (`./screenshot.sh` shares the port; use a serial term or the
scratch python that writes `pmu\n` and reads lines)

- `boots` — power-on count since reset.
- `bootmv` / `minbootmv` — battery mV at the latest / lowest boot (cold-wake voltage).
- `max_prior_session` — longest session uptime (s) before a boot; if it grows to
  hours across a supposed power-off, the device never stopped.
- `live_uptime` / `lastmv` — current session uptime + last 60 s battery mV.
- `triggers: pwr_actions / touch_press_edges / touch_clicks` — what drove the
  screen toggle (this is how we proved the storm was PWR, not touch).

### Key results captured (so a re-run can be compared)

- Storm = `pwr_actions=11, touch=0` on a cold boot → fixed → `2` → (tightened) `0`.
- Cell healthy through cycles: `minbootmv≈4044 mV`; power-off truly off for 20 h.
- `pmu.shutdown()` powers off cleanly on USB, no VBUS auto-reboot.

## Code blocks to restore

### `firmware/src/hal/power_hal.h`
```c
#include <stdint.h>   // for uint16_t below

void power_hal_debug_dump(void);   // dump PMU rail/charger state to Serial
uint16_t power_hal_batt_mv(void);  // raw battery voltage (mV)
```

### `firmware/src/boards/waveshare_amoled_18/power.cpp`
```c
void power_hal_debug_dump(void) {
    Serial.println("---- AXP2101 dump ----");
    Serial.printf("vbus_in=%d  vbus_mV=%u\n", (int)pmu.isVbusIn(), pmu.getVbusVoltage());
    Serial.printf("batt_connect=%d  batt_mV=%u  pct=%d\n",
                  (int)pmu.isBatteryConnect(), pmu.getBattVoltage(), pmu.getBatteryPercent());
    Serial.printf("charging=%d  charger_status=%d\n",
                  (int)pmu.isCharging(), (int)pmu.getChargerStatus());
    Serial.printf("sys_powerdown_mV=%u\n", pmu.getSysPowerDownVoltage());
    Serial.println("----------------------");
}
uint16_t power_hal_batt_mv(void) { return pmu.getBattVoltage(); }
```

### `firmware/src/ui.h` + `firmware/src/ui.cpp`
```c
// ui.h
uint32_t ui_debug_click_count(void);

// ui.cpp — counts touch-CLICK toggles in global_click_cb
static uint32_t s_dbg_click_count = 0;
uint32_t ui_debug_click_count(void) { return s_dbg_click_count; }
// inside global_click_cb(): s_dbg_click_count++;
```

### `firmware/src/main.cpp`
```c
#include <Preferences.h>
static Preferences diag_prefs;
static volatile uint32_t dbg_pwr_act   = 0;  // PWR presses that caused a screen action
static volatile uint32_t dbg_tch_press = 0;  // touch press edges seen

// --- in my_touch_cb(), right after `const bool raw_pressed = pressed;` ---
{ static bool tw = false; if (raw_pressed && !tw) dbg_tch_press++; tw = raw_pressed; }

// --- in the PWR block, right after `if (!idle_consume_wake_press()) {` ---
dbg_pwr_act++;

// --- in setup(), right after power_hal_init() : NVS flight-recorder ---
{
    diag_prefs.begin("diag", false);
    uint32_t boots   = diag_prefs.getULong("boots", 0) + 1;
    uint32_t lastses = diag_prefs.getULong("uptime", 0);
    uint32_t prevmax = diag_prefs.getULong("prevup", 0);
    if (lastses > prevmax) prevmax = lastses;
    uint16_t bmv     = power_hal_batt_mv();
    uint16_t minmv   = diag_prefs.getUShort("minbootmv", 0xFFFF);
    if (bmv > 0 && bmv < minmv) minmv = bmv;
    diag_prefs.putULong("boots", boots);
    diag_prefs.putULong("prevup", prevmax);
    diag_prefs.putUShort("bootmv", bmv);
    diag_prefs.putUShort("minbootmv", minmv);
    diag_prefs.putULong("uptime", 0);
    diag_prefs.end();
    Serial.printf("DIAG boot #%lu  bootmv=%u mV  minbootmv=%u mV  max_prior_session=%lu s\n",
                  (unsigned long)boots, bmv, minmv, (unsigned long)prevmax);
}

// --- in loop(), after check_serial_cmd() : persist uptime every 60s ---
{
    static uint32_t last_diag_ms = 0;
    if (millis() - last_diag_ms >= 60000) {
        last_diag_ms = millis();
        diag_prefs.begin("diag", false);
        diag_prefs.putULong("uptime", millis() / 1000);
        diag_prefs.putUShort("lastmv", power_hal_batt_mv());
        diag_prefs.end();
    }
}

// --- in check_serial_cmd(), as extra `else if` branches ---
else if (strcmp(cmd_buf, "pmu") == 0) {
    diag_prefs.begin("diag", true);
    Serial.printf("DIAG boots=%lu  bootmv=%u mV  minbootmv=%u mV  max_prior_session=%lu s  live_uptime=%lu s  lastmv=%u mV\n",
                  (unsigned long)diag_prefs.getULong("boots", 0),
                  diag_prefs.getUShort("bootmv", 0),
                  diag_prefs.getUShort("minbootmv", 0),
                  (unsigned long)diag_prefs.getULong("prevup", 0),
                  (unsigned long)diag_prefs.getULong("uptime", 0),
                  diag_prefs.getUShort("lastmv", 0));
    diag_prefs.end();
    Serial.printf("DIAG triggers: pwr_actions=%lu  touch_press_edges=%lu  touch_clicks=%lu\n",
                  (unsigned long)dbg_pwr_act, (unsigned long)dbg_tch_press,
                  (unsigned long)ui_debug_click_count());
    power_hal_debug_dump();
}
else if (strcmp(cmd_buf, "off") == 0) {
    Serial.println("DIAG calling power_hal_shutdown()");
    delay(50);
    power_hal_shutdown();
}
else if (strcmp(cmd_buf, "diagreset") == 0) {
    diag_prefs.begin("diag", false);
    diag_prefs.clear();
    diag_prefs.end();
    Serial.println("DIAG cleared");
}
```

> Note: `power_hal_debug_dump` / `power_hal_batt_mv` were only implemented for
> the AMOLED-1.8 board. Building other envs with these declared in the HAL needs
> stubs in their `power.cpp`.
