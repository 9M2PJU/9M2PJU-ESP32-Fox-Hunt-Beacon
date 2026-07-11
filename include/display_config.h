#pragma once

// Display type selection. Set via build flags in platformio.ini per board.
//   0 = no display (default)
//   1 = OLED SSD1306 via U8g2 (I2C)
//   2 = TFT via TFT_eSPI (SPI, ST7789/ST7735)
//   3 = E-Ink SSD1680 via GxEPD2 (SPI)
#ifndef DISPLAY_TYPE
#define DISPLAY_TYPE 0
#endif

// --- OLED defaults (overridden by build flags per board) ---
#ifndef OLED_SDA
#define OLED_SDA 4
#endif
#ifndef OLED_SCL
#define OLED_SCL 15
#endif
#ifndef OLED_RST
#define OLED_RST 16
#endif
#ifndef OLED_WIDTH
#define OLED_WIDTH 128
#endif
#ifndef OLED_HEIGHT
#define OLED_HEIGHT 64
#endif
#ifndef OLED_I2C_ADDR
#define OLED_I2C_ADDR 0x3C
#endif

// --- TFT defaults (configured via TFT_eSPI build flags per board) ---
#ifndef TFT_BL
#define TFT_BL -1
#endif
