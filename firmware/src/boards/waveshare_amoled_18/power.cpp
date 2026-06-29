#include "../../hal/power_hal.h"
#include "board.h"
#include "io_expander.h"
#include <Arduino.h>
#include <Wire.h>
#include <XPowersLib.h>

// PWR button comes from XCA9554 EXIO4 (active HIGH). The PMU still
// provides battery monitoring; we just don't subscribe to its PKEY IRQ.
//
// The AXP2101 PKEY IRQs (short/long/release) aren't available here, so we
// synthesize the same three edges in software from the polled EXIO4 level:
//   short    — fired on release if the hold was shorter than PWR_LONG_MS
//   long     — fired once when a hold crosses PWR_LONG_MS
//   release  — fired on every falling edge
// This keeps the hold-to-pair gesture logic in main.cpp board-agnostic.

#define BATTERY_POLL_MS  2000
#define CHARGING_POLL_MS 500
#define PWR_POLL_MS      50
#define PWR_LONG_MS      1500   // hold threshold, mirrors the AXP LONG IRQ
// EXIO4 emits a burst of phantom edges in the first ~second after a cold
// (AXP power-on) boot — it fired ~11 spurious short-presses, storming the
// screen toggle. Ignore PWR events until the line settles past this window.
#define PWR_BOOT_GRACE_MS 2000

static XPowersPMU pmu;

static int      cached_pct        = -1;
static bool     cached_charging   = false;
static bool     cached_vbus       = false;
static bool     pwr_pressed_flag  = false;
static bool     pwr_long_flag     = false;
static bool     pwr_released_flag = false;
static bool     last_pwr_state    = false;   // edge detector for EXIO4 (debounced level)
static bool     pwr_seeded        = false;   // last_pwr_state seeded from the real pin yet?
static uint32_t pwr_press_started_ms = 0;
static bool     pwr_long_fired    = false;   // long already fired for this hold
static uint32_t last_battery_ms   = 0;
static uint32_t last_charging_ms  = 0;
static uint32_t last_pwr_ms       = 0;

void power_hal_init(void) {
    if (!pmu.begin(Wire, AXP2101_ADDR, IIC_SDA, IIC_SCL)) {
        Serial.println("AXP2101 init failed");
        return;
    }
    Serial.println("AXP2101 init OK");

    pmu.enableBattDetection();
    pmu.enableBattVoltageMeasure();
    // No PMU IRQ wiring — PWR comes via io_expander_get() below.

    cached_charging = pmu.isCharging();
    cached_vbus     = pmu.isVbusIn();
    cached_pct = pmu.getBatteryPercent();
}

void power_hal_tick(void) {
    uint32_t now = millis();

    if (now - last_charging_ms >= CHARGING_POLL_MS) {
        last_charging_ms = now;
        cached_charging = pmu.isCharging();
        cached_vbus     = pmu.isVbusIn();
    }
    if (now - last_battery_ms >= BATTERY_POLL_MS) {
        last_battery_ms = now;
        cached_pct = pmu.getBatteryPercent();
    }
    if (now - last_pwr_ms >= PWR_POLL_MS) {
        last_pwr_ms = now;

        // Debounce: only treat a level as real once three consecutive samples
        // (~150ms) agree. Kills the fast edge noise EXIO4 emits at boot.
        bool raw = io_expander_get(IOX_PIN_PWR_BTN);
        static bool    deb_prev  = false;
        static uint8_t deb_run   = 0;
        static bool    deb_level = false;
        if (raw == deb_prev) { if (deb_run < 255) deb_run++; } else { deb_run = 0; }
        deb_prev = raw;
        if (deb_run >= 2) deb_level = raw;   // 3 samples in a row → confirmed level
        bool pwr_now = deb_level;

        if (!pwr_seeded) { last_pwr_state = pwr_now; pwr_seeded = true; }

        if (now < PWR_BOOT_GRACE_MS) {
            // Boot grace: follow the level but emit no edges, and drain any
            // flags that slipped through — discards the cold-boot phantom storm.
            last_pwr_state    = pwr_now;
            pwr_pressed_flag  = false;
            pwr_long_flag     = false;
            pwr_released_flag = false;
        } else if (pwr_now && !last_pwr_state) {     // rising edge — hold begins
            pwr_press_started_ms = now;
            pwr_long_fired = false;
            last_pwr_state = pwr_now;
        } else if (pwr_now && last_pwr_state) {      // held
            if (!pwr_long_fired && (now - pwr_press_started_ms >= PWR_LONG_MS)) {
                pwr_long_flag  = true;
                pwr_long_fired = true;
            }
        } else if (!pwr_now && last_pwr_state) {     // falling edge — release
            pwr_released_flag = true;
            if (!pwr_long_fired) pwr_pressed_flag = true;  // short press
            last_pwr_state = pwr_now;
        }
    }
}

int  power_hal_battery_pct(void) { return cached_pct; }
bool power_hal_is_charging(void) { return cached_charging; }
bool power_hal_is_vbus_in(void)  { return cached_vbus; }

bool power_hal_pwr_pressed(void) {
    if (pwr_pressed_flag) { pwr_pressed_flag = false; return true; }
    return false;
}

bool power_hal_pwr_long_pressed(void) {
    if (pwr_long_flag) { pwr_long_flag = false; return true; }
    return false;
}

bool power_hal_pwr_released(void) {
    if (pwr_released_flag) { pwr_released_flag = false; return true; }
    return false;
}

void power_hal_shutdown(void) {
    Serial.println("AXP2101 shutdown");
    pmu.shutdown();
}
