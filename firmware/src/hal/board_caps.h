#pragma once
#include <stdint.h>

// Runtime board description consumed by board-agnostic code (UI, main loop).
// Each board provides a single BoardCaps instance via board_caps().
//
// Compile-time-only facts (pin numbers, library choice) belong in
// boards/<name>/board.h and never leak into shared code. Anything the UI or
// main loop needs at runtime — display size, optional-feature presence —
// goes here so shared code stays free of #ifdef BOARD_*.
struct BoardCaps {
    const char* name;        // human-readable, e.g. "Waveshare AMOLED 2.16"

    int16_t width;           // active display width in pixels
    int16_t height;          // active display height in pixels

    uint8_t button_count;    // 1 = primary (BOOT) only; 2 = primary + secondary
    bool    has_rotation;    // IMU-driven CPU rotation in the flush callback
    bool    has_battery;     // AXP2101 battery measurement is meaningful
    bool    has_imu;         // QMI8658 (or compatible) is populated

    // PWR press toggles splash <-> usage instead of cycling animations /
    // brightness. For boards whose touch can't switch screens (so PWR is the
    // only on-device way to reach the usage view). Defaults false.
    bool    pwr_toggles_screen;

    // Primary (BOOT) button sends Shift+Tab (Claude Code mode cycle / auto-
    // accept) instead of Space (voice PTT). For boards where the single front
    // button is better spent on mode toggling than push-to-talk.
    bool    primary_sends_shift_tab;

    // Long PWR hold triggers power_hal_shutdown() (AXP soft power-off). For
    // boards whose PWR button isn't on the PMU power key, so the PMU's own
    // long-press hardware shutdown never fires.
    bool    pwr_software_shutdown;
};

const BoardCaps& board_caps(void);
