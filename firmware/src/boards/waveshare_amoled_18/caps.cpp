#include "../../hal/board_caps.h"
#include "board.h"

static const BoardCaps caps = {
    .name = BOARD_NAME,
    .width = LCD_WIDTH,
    .height = LCD_HEIGHT,
    .button_count = 1,
    .has_rotation = false,
    .has_battery = true,
    .has_imu = true,
    // CST816 (V2 panel) touch can't switch screens yet, so PWR toggles them.
    .pwr_toggles_screen = true,
    // Single front (BOOT) button → Shift+Tab for Claude Code mode cycling.
    .primary_sends_shift_tab = true,
    // PWR is on EXIO4, not the AXP power key, so do a software power-off.
    .pwr_software_shutdown = true,
};

const BoardCaps& board_caps(void) { return caps; }
