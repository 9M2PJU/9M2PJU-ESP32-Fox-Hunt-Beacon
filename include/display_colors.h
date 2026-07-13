#pragma once

// Display color theme. Shared across all display backends.
//
// For TFT (RGB565 16-bit) these are real colors. For OLED and E-Ink
// (monochrome) only the foreground/background distinction matters; accent
// colors are emulated with inverted text or bold fonts where available.
//
// Color values adapted from the LoRa_APRS_Tracker project (CA2RXU) which
// uses a navy-blue header bar, content-based body coloring, and a 2px
// status accent bar on the left edge of the screen.

// === RGB565 colors (TFT) ===
#define FOX_BLACK       0x0000
#define FOX_WHITE       0xFFFF
#define FOX_NAVY        0x000F   // header background
#define FOX_YELLOW      0xFFE0   // header text, selected menu item, GPS coords
#define FOX_CYAN        0x07FF   // default body text, info
#define FOX_GREEN       0x1482   // TX active, OK states (custom green)
#define FOX_GREEN_DARK  0x0b22
#define FOX_RED         0xc8a2   // low battery, warnings (custom red)
#define FOX_RED_LIGHT   0xd8c4
#define FOX_ORANGE      0xFD20   // battery info, button hints
#define FOX_GREY        0x7BEF   // idle, last RX (custom grey)
#define FOX_GREY_DARK   0x39E7
#define FOX_MAGENTA     0xF81F   // special accents

// === Semantic roles ===
// Used by all backends; TFT maps them to RGB565, OLED/E-Ink map to
// foreground/background/inverted.
#define COLOR_BG          FOX_BLACK
#define COLOR_HEADER_BG   FOX_NAVY
#define COLOR_HEADER_FG   FOX_YELLOW
#define COLOR_BODY        FOX_CYAN
#define COLOR_ACCENT      FOX_YELLOW
#define COLOR_SUCCESS     FOX_GREEN
#define COLOR_WARNING     FOX_ORANGE
#define COLOR_ERROR       FOX_RED
#define COLOR_IDLE        FOX_GREY
#define COLOR_HINT        FOX_ORANGE   // button hints, footer
