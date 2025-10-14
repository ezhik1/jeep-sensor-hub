#ifndef PALETTE_H
#define PALETTE_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Global Color Palette
#define PALETTE_WHITE lv_color_hex(0xFFFFFF)
#define PALETTE_BLACK lv_color_hex(0x000000)
#define PALETTE_GRAY lv_color_hex(0x8c8c8c)
#define PALETTE_YELLOW lv_color_hex(0xFFD700)
#define PALETTE_RED lv_color_hex(0xff0000)
#define PALETTE_GREEN lv_color_hex(0x00ff00)
#define PALETTE_BLUE lv_color_hex(0x004557)
#define PALETTE_CYAN lv_color_hex(0x00FFFF)
#define PALETTE_DARK_GRAY lv_color_hex(0x3F3F3F)
#define PALETTE_BROWN lv_color_hex(0x793100)
#define PALETTE_ORANGE lv_color_hex(0xFF8C00)

// Shared Color Aliases
#define BORDER_COLOR PALETTE_WHITE
#define TEXT_COLOR_LIGHT PALETTE_WHITE
#define TEXT_COLOR_DARK PALETTE_DARK_GRAY
#define BACKGROUND_COLOR PALETTE_BLACK

#ifdef __cplusplus
}
#endif

#endif // PALETTE_H
