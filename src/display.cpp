#include "display.h"

#if DISPLAY_TYPE == 1

// --- OLED SSD1306 via U8g2 ---
// Monochrome adaptation of the color theme:
//   - Inverted header bar (white box, black text) emulates navy blue header
//   - 2px status accent bar on left edge (solid=TX, dashed=idle, etc.)
//   - State line uses inverted text for TX/LOWBAT emphasis
#include <U8g2lib.h>
#include <Wire.h>
#include "display_colors.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);
static bool displayOn = true;

// Draw the inverted header bar (emulates navy blue header on TFT).
// Fills a black box and draws text in white-on-black by inverting: we draw
// a filled box, then use setDrawColor(0) to erase text pixels inside it.
static void drawHeader(const char *text) {
  u8g2.setFont(u8g2_font_8x13B_tr);
  int w = u8g2.getStrWidth(text);
  u8g2.drawBox(0, 0, w + 4, 15);
  u8g2.setDrawColor(0);
  u8g2.drawStr(2, 12, text);
  u8g2.setDrawColor(1);
}

// Draw a 2px-wide status accent bar on the left edge.
// Pattern encodes state: solid=TX/BEACON, dashed=IDLE, double=LOWBAT, dotted=STARTUP
static void drawAccentBar(const char *stateStr) {
  String s = stateStr;
  s.toUpperCase();
  if (s == "TX" || s == "BEACON") {
    u8g2.drawBox(0, 0, 2, 64);              // solid
  } else if (s == "LOWBAT") {
    for (int y = 0; y < 64; y += 6)         // double dashes
      u8g2.drawBox(0, y, 2, 3);
  } else if (s == "STARTUP") {
    for (int y = 0; y < 64; y += 4)         // dotted
      u8g2.drawBox(0, y, 2, 2);
  } else {
    for (int y = 0; y < 64; y += 8)         // dashed (IDLE)
      u8g2.drawBox(0, y, 2, 4);
  }
}

void displayInit(const char *callsign, const char *version) {
  Wire.begin(OLED_SDA, OLED_SCL);
  u8g2.begin();
  displayStartupScreen(callsign, version);
}

void displayStartupScreen(const char *callsign, const char *version) {
  u8g2.clearBuffer();
  char header[24];
  snprintf(header, sizeof(header), "%s Fox", callsign);
  drawHeader(header);

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.setCursor(0, 34);
  u8g2.print("v");
  u8g2.print(version);
  u8g2.setCursor(0, 50);
  u8g2.print("Starting...");
  u8g2.sendBuffer();
}

void displayUpdate(const char *callsign, const char *foxId,
                   const char *modeStr, const char *stateStr,
                   const char *timingStr, const char *batteryStr,
                   const char *ipStr) {
  if (!displayOn) return;
  u8g2.clearBuffer();

  // Status accent bar (left edge)
  drawAccentBar(stateStr);

  // Inverted header bar with callsign + fox ID
  char header[24];
  snprintf(header, sizeof(header), "%s %s", callsign, foxId);
  drawHeader(header);

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.setCursor(0, 26);
  u8g2.print(modeStr);

  // State line: inverted (highlighted) for TX/BEACON/LOWBAT, plain for others
  String state = stateStr;
  state.toUpperCase();
  bool highlight = (state == "TX" || state == "BEACON" || state == "LOWBAT");
  if (highlight) {
    int w = u8g2.getStrWidth(stateStr);
    u8g2.drawBox(0, 36, w + 4, 11);
    u8g2.setDrawColor(0);
    u8g2.drawStr(2, 45, stateStr);
    u8g2.setDrawColor(1);
  } else {
    u8g2.setCursor(0, 45);
    u8g2.print(stateStr);
  }

  u8g2.setCursor(0, 56);
  u8g2.print(timingStr);
  u8g2.setCursor(0, 63);
  if (batteryStr[0]) {
    u8g2.print(batteryStr);
    u8g2.print("  ");
  }
  u8g2.print(ipStr);
  u8g2.sendBuffer();
}

void displayMenu(int selectedIndex, int itemCount,
                 const char *const labels[], const char *const values[]) {
  if (!displayOn) return;
  u8g2.clearBuffer();

  // Inverted header bar
  drawHeader("Settings");
  u8g2.setFont(u8g2_font_6x10_tr);

  // Show up to 5 items, scrolling if needed
  int visibleStart = 0;
  if (itemCount > 5 && selectedIndex > 3) {
    visibleStart = selectedIndex - 3;
    if (visibleStart > itemCount - 5) visibleStart = itemCount - 5;
  }
  int visibleCount = (itemCount < 5) ? itemCount : 5;

  for (int i = 0; i < visibleCount; i++) {
    int idx = visibleStart + i;
    if (idx >= itemCount) break;
    int y = 26 + i * 10;
    if (idx == selectedIndex) {
      // Inverted highlight for selected item
      char line[24];
      snprintf(line, sizeof(line), "%-10s %s", labels[idx], values[idx]);
      int w = u8g2.getStrWidth(line);
      u8g2.drawBox(0, y - 8, w + 4, 10);
      u8g2.setDrawColor(0);
      u8g2.drawStr(2, y, line);
      u8g2.setDrawColor(1);
    } else {
      char line[24];
      snprintf(line, sizeof(line), "%-10s %s", labels[idx], values[idx]);
      u8g2.drawStr(0, y, line);
    }
  }

  // Footer hint
  u8g2.drawStr(0, 63, "1=Next 2=Toggle Hold=Exit");
  u8g2.sendBuffer();
}

void displayAPMode(const char *ssid, const char *ip) {
  if (!displayOn) return;
  u8g2.clearBuffer();
  drawHeader("AP MODE");
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.setCursor(0, 28);
  u8g2.print("SSID:");
  u8g2.setCursor(0, 40);
  u8g2.print(ssid);
  u8g2.setCursor(0, 52);
  u8g2.print(ip);
  u8g2.setCursor(0, 63);
  u8g2.print("Web admin ready");
  u8g2.sendBuffer();
}

void displayPower(bool on) {
  displayOn = on;
  if (on) {
    u8g2.setPowerSave(0);
  } else {
    u8g2.setPowerSave(1);
  }
}

void displayClear() {
  u8g2.clearBuffer();
  u8g2.sendBuffer();
}

#elif DISPLAY_TYPE == 2

// --- TFT ST7789/ST7735 via TFT_eSPI ---
// Color theme adapted from LoRa_APRS_Tracker (CA2RXU):
//   - Navy blue header bar with yellow text
//   - Content-based body coloring (TX=green, LOWBAT=red, battery=orange)
//   - 2px status accent bar on left edge (green=TX, yellow=idle, red=lowbat)
#include <TFT_eSPI.h>
#include "display_colors.h"

TFT_eSPI tft = TFT_eSPI();
static bool displayOn = true;

// Pick a status accent bar color from the beacon state string.
static uint16_t stateAccentColor(const char *stateStr) {
  String s = stateStr;
  s.toUpperCase();
  if (s == "TX" || s == "BEACON")      return FOX_GREEN;
  if (s == "LOWBAT")                    return FOX_RED;
  if (s == "STARTUP")                   return FOX_ORANGE;
  return FOX_GREY;  // IDLE
}

// Pick a body text color for the state line.
static uint16_t stateTextColor(const char *stateStr) {
  String s = stateStr;
  s.toUpperCase();
  if (s == "TX" || s == "BEACON")      return FOX_GREEN;
  if (s == "LOWBAT")                    return FOX_RED;
  if (s == "STARTUP")                   return FOX_ORANGE;
  return FOX_CYAN;  // IDLE
}

void displayInit(const char *callsign, const char *version) {
  tft.init();
  tft.setRotation(1);
  displayStartupScreen(callsign, version);
}

void displayStartupScreen(const char *callsign, const char *version) {
  tft.fillScreen(COLOR_BG);
  tft.setTextDatum(TL_DATUM);

  // Navy blue header bar with yellow text
  tft.fillRect(0, 0, tft.width(), 30, COLOR_HEADER_BG);
  tft.setTextColor(COLOR_HEADER_FG, COLOR_HEADER_BG);
  String header = String(callsign) + " Fox";
  tft.drawString(header, 4, 4, 4);

  // Body
  tft.setTextColor(FOX_CYAN, COLOR_BG);
  String ver = "v" + String(version);
  tft.drawString(ver, 4, 40, 2);
  tft.setTextColor(FOX_ORANGE, COLOR_BG);
  tft.drawString("Starting...", 4, 60, 2);
}

void displayUpdate(const char *callsign, const char *foxId,
                   const char *modeStr, const char *stateStr,
                   const char *timingStr, const char *batteryStr,
                   const char *ipStr) {
  if (!displayOn) return;
  tft.fillScreen(COLOR_BG);
  tft.setTextDatum(TL_DATUM);

  // 2px status accent bar on left edge
  tft.fillRect(0, 0, 2, tft.height(), stateAccentColor(stateStr));

  // Navy blue header bar with yellow callsign + fox ID
  tft.fillRect(2, 0, tft.width() - 2, 30, COLOR_HEADER_BG);
  tft.setTextColor(COLOR_HEADER_FG, COLOR_HEADER_BG);
  tft.drawString(String(callsign), 6, 4, 4);
  tft.drawString(String(foxId), 90, 4, 4);

  // Mode line (yellow accent)
  tft.setTextColor(FOX_YELLOW, COLOR_BG);
  tft.drawString(String(modeStr), 4, 36, 2);

  // State line (color-coded by state)
  tft.setTextColor(stateTextColor(stateStr), COLOR_BG);
  tft.drawString(String(stateStr), 4, 56, 2);

  // Timing line (cyan)
  tft.setTextColor(FOX_CYAN, COLOR_BG);
  tft.drawString(String(timingStr), 4, 76, 2);

  // Battery line (orange if present)
  if (batteryStr[0]) {
    tft.setTextColor(FOX_ORANGE, COLOR_BG);
    tft.drawString(String(batteryStr), 4, 96, 2);
  }

  // IP line (green)
  if (ipStr[0]) {
    tft.setTextColor(FOX_GREEN, COLOR_BG);
    tft.drawString(String(ipStr), 4, 116, 2);
  }
}

void displayMenu(int selectedIndex, int itemCount,
                 const char *const labels[], const char *const values[]) {
  if (!displayOn) return;
  tft.fillScreen(COLOR_BG);
  tft.setTextDatum(TL_DATUM);

  // Navy blue header bar
  tft.fillRect(0, 0, tft.width(), 30, COLOR_HEADER_BG);
  tft.setTextColor(COLOR_HEADER_FG, COLOR_HEADER_BG);
  tft.drawString("Settings", 4, 4, 4);

  tft.setTextColor(FOX_CYAN, COLOR_BG);

  int visibleStart = 0;
  if (itemCount > 6 && selectedIndex > 4) {
    visibleStart = selectedIndex - 4;
    if (visibleStart > itemCount - 6) visibleStart = itemCount - 6;
  }
  int visibleCount = (itemCount < 6) ? itemCount : 6;

  for (int i = 0; i < visibleCount; i++) {
    int idx = visibleStart + i;
    if (idx >= itemCount) break;
    int y = 40 + i * 22;
    if (idx == selectedIndex) {
      tft.setTextColor(FOX_YELLOW, COLOR_BG);
      tft.drawString(">", 4, y, 2);
    } else {
      tft.setTextColor(FOX_WHITE, COLOR_BG);
    }
    char line[32];
    snprintf(line, sizeof(line), "%-12s %s", labels[idx], values[idx]);
    tft.drawString(line, 18, y, 2);
    tft.setTextColor(FOX_WHITE, COLOR_BG);
  }

  // Footer hint (orange)
  tft.setTextColor(FOX_ORANGE, COLOR_BG);
  tft.drawString("1=Next 2=Toggle Hold=Exit", 4, tft.height() - 16, 1);
}

void displayAPMode(const char *ssid, const char *ip) {
  if (!displayOn) return;
  tft.fillScreen(COLOR_BG);
  tft.setTextDatum(TL_DATUM);

  // Navy blue header bar
  tft.fillRect(0, 0, tft.width(), 30, COLOR_HEADER_BG);
  tft.setTextColor(COLOR_HEADER_FG, COLOR_HEADER_BG);
  tft.drawString("AP MODE", 4, 4, 4);

  tft.setTextColor(FOX_CYAN, COLOR_BG);
  tft.drawString("SSID:", 4, 40, 2);
  tft.setTextColor(FOX_WHITE, COLOR_BG);
  tft.drawString(String(ssid), 4, 58, 2);
  tft.setTextColor(FOX_GREEN, COLOR_BG);
  tft.drawString(String(ip), 4, 78, 2);
  tft.setTextColor(FOX_ORANGE, COLOR_BG);
  tft.drawString("Web admin ready", 4, 98, 2);
}

void displayPower(bool on) {
  displayOn = on;
  if (on) {
    tft.fillScreen(COLOR_BG);
  }
  // TFT_eSPI doesn't have a simple power-off; we just stop drawing.
  // Backlight control would need a BL pin PWM which varies per board.
}

void displayClear() {
  tft.fillScreen(COLOR_BG);
}

#elif DISPLAY_TYPE == 3

// --- E-Ink SSD1680 via GxEPD2 ---
// Monochrome adaptation of the color theme (same approach as OLED):
//   - Inverted header bar (black box, white text) emulates navy blue header
//   - 2px status accent bar on left edge (pattern-coded by state)
//   - State line uses inverted text for TX/LOWBAT emphasis
#include <GxEPD2_BW.h>
#include <SPI.h>
#include "display_colors.h"

// Select the correct panel class based on resolution
#if EINK_WIDTH == 250 && EINK_HEIGHT == 122
  // 2.13" BW (250x122) — DEPG0213BN / SSD1680
  static GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> eink(
      GxEPD2_213_BN(EINK_CS, EINK_DC, EINK_RST, EINK_BUSY));
#elif EINK_WIDTH == 296 && EINK_HEIGHT == 128
  // 2.9" BW (296x128) — DEPG0290BS / SSD1680
  static GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> eink(
      GxEPD2_290_BS(EINK_CS, EINK_DC, EINK_RST, EINK_BUSY));
#else
  #error "Unsupported E-Ink resolution. Add a panel class for this size."
#endif

static bool displayOn = true;
static bool firstRefresh = true;

// Draw inverted header bar (black box with white text) — emulates navy blue
// header on TFT. GxEPD2 uses BLACK as foreground; we fill a rect then write
// text in WHITE over it.
static void einkDrawHeader(const char *text) {
  int16_t x1, y1;
  uint16_t w, h;
  eink.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  eink.fillRect(0, 0, w + 4, h + 2, GxEPD_BLACK);
  eink.setTextColor(GxEPD_WHITE);
  eink.setCursor(2, 0);
  eink.print(text);
  eink.setTextColor(GxEPD_BLACK);
}

// Draw 2px status accent bar on left edge, pattern-coded by state.
static void einkDrawAccentBar(const char *stateStr) {
  String s = stateStr;
  s.toUpperCase();
  const int h = EINK_HEIGHT;
  if (s == "TX" || s == "BEACON") {
    eink.fillRect(0, 0, 2, h, GxEPD_BLACK);           // solid
  } else if (s == "LOWBAT") {
    for (int y = 0; y < h; y += 6)                     // double dashes
      eink.fillRect(0, y, 2, 3, GxEPD_BLACK);
  } else if (s == "STARTUP") {
    for (int y = 0; y < h; y += 4)                     // dotted
      eink.fillRect(0, y, 2, 2, GxEPD_BLACK);
  } else {
    for (int y = 0; y < h; y += 8)                     // dashed (IDLE)
      eink.fillRect(0, y, 2, 4, GxEPD_BLACK);
  }
}

static void einkRefresh() {
  if (firstRefresh) {
    eink.display(false);
    firstRefresh = false;
  } else {
    eink.display(true);  // partial refresh for speed
  }
}

static void einkPowerOn() {
#ifdef EINK_VEXT
  #ifdef EINK_VEXT_ACTIVE_LOW
    pinMode(EINK_VEXT, OUTPUT);
    digitalWrite(EINK_VEXT, LOW);  // active LOW: LOW = ON
  #else
    pinMode(EINK_VEXT, OUTPUT);
    digitalWrite(EINK_VEXT, HIGH);  // active HIGH: HIGH = ON
  #endif
  delay(10);
#endif
}

void displayInit(const char *callsign, const char *version) {
  einkPowerOn();
  SPI.begin(EINK_SCLK, -1, EINK_MOSI, EINK_CS);
  eink.init();
  eink.setRotation(1);
  eink.setTextColor(GxEPD_BLACK);
  eink.setTextSize(1);
  displayStartupScreen(callsign, version);
}

void displayStartupScreen(const char *callsign, const char *version) {
  eink.setFullWindow();
  eink.fillScreen(GxEPD_WHITE);
  char header[24];
  snprintf(header, sizeof(header), "%s Fox", callsign);
  einkDrawHeader(header);
  eink.setCursor(0, 16);
  eink.print("v");
  eink.print(version);
  eink.setCursor(0, 32);
  eink.print("Starting...");
  einkRefresh();
}

void displayUpdate(const char *callsign, const char *foxId,
                   const char *modeStr, const char *stateStr,
                   const char *timingStr, const char *batteryStr,
                   const char *ipStr) {
  if (!displayOn) return;
  eink.setFullWindow();
  eink.fillScreen(GxEPD_WHITE);

  // Status accent bar (left edge)
  einkDrawAccentBar(stateStr);

  // Inverted header bar
  char header[24];
  snprintf(header, sizeof(header), "%s %s", callsign, foxId);
  einkDrawHeader(header);

  eink.setCursor(0, 16);
  eink.print(modeStr);

  // State line: inverted for TX/BEACON/LOWBAT
  String state = stateStr;
  state.toUpperCase();
  if (state == "TX" || state == "BEACON" || state == "LOWBAT") {
    int16_t x1, y1;
    uint16_t w, h;
    eink.getTextBounds(stateStr, 0, 0, &x1, &y1, &w, &h);
    eink.fillRect(0, 26, w + 4, h + 2, GxEPD_BLACK);
    eink.setTextColor(GxEPD_WHITE);
    eink.setCursor(2, 26);
    eink.print(stateStr);
    eink.setTextColor(GxEPD_BLACK);
  } else {
    eink.setCursor(0, 28);
    eink.print(stateStr);
  }

  eink.setCursor(0, 40);
  eink.print(timingStr);
  eink.setCursor(0, 52);
  if (batteryStr[0]) {
    eink.print(batteryStr);
    eink.print("  ");
  }
  eink.print(ipStr);
  einkRefresh();
}

void displayMenu(int selectedIndex, int itemCount,
                 const char *const labels[], const char *const values[]) {
  if (!displayOn) return;
  eink.setFullWindow();
  eink.fillScreen(GxEPD_WHITE);
  einkDrawHeader("Settings");

  int visibleStart = 0;
  if (itemCount > 5 && selectedIndex > 3) {
    visibleStart = selectedIndex - 3;
    if (visibleStart > itemCount - 5) visibleStart = itemCount - 5;
  }
  int visibleCount = (itemCount < 5) ? itemCount : 5;

  for (int i = 0; i < visibleCount; i++) {
    int idx = visibleStart + i;
    if (idx >= itemCount) break;
    int y = 24 + i * 10;
    if (idx == selectedIndex) {
      // Inverted highlight for selected item
      char line[24];
      snprintf(line, sizeof(line), "%s %s", labels[idx], values[idx]);
      int16_t x1, y1;
      uint16_t w, h;
      eink.getTextBounds(line, 0, 0, &x1, &y1, &w, &h);
      eink.fillRect(0, y - 8, w + 4, h + 2, GxEPD_BLACK);
      eink.setTextColor(GxEPD_WHITE);
      eink.setCursor(2, y - 8);
      eink.print(line);
      eink.setTextColor(GxEPD_BLACK);
    } else {
      eink.setCursor(4, y);
      eink.print(labels[idx]);
      eink.print(" ");
      eink.print(values[idx]);
    }
  }

  eink.setCursor(0, EINK_HEIGHT - 8);
  eink.print("1=Next 2=Toggle Hold=Exit");
  einkRefresh();
}

void displayAPMode(const char *ssid, const char *ip) {
  if (!displayOn) return;
  eink.setFullWindow();
  eink.fillScreen(GxEPD_WHITE);
  einkDrawHeader("AP MODE");
  eink.setCursor(0, 24);
  eink.print("SSID:");
  eink.setCursor(0, 36);
  eink.print(ssid);
  eink.setCursor(0, 48);
  eink.print(ip);
  eink.setCursor(0, 60);
  eink.print("Web admin ready");
  einkRefresh();
}

void displayPower(bool on) {
  displayOn = on;
#ifdef EINK_VEXT
  if (!on) {
    #ifdef EINK_VEXT_ACTIVE_LOW
      digitalWrite(EINK_VEXT, HIGH);  // active LOW: HIGH = OFF
    #else
      digitalWrite(EINK_VEXT, LOW);   // active HIGH: LOW = OFF
    #endif
  } else {
    einkPowerOn();
  }
#endif
}

void displayClear() {
  eink.setFullWindow();
  eink.fillScreen(GxEPD_WHITE);
  eink.display(false);
}

#else

// --- No display ---
void displayInit(const char *, const char *) {}
void displayStartupScreen(const char *, const char *) {}
void displayUpdate(const char *, const char *, const char *, const char *,
                   const char *, const char *, const char *) {}
void displayMenu(int, int, const char *const[], const char *const[]) {}
void displayAPMode(const char *, const char *) {}
void displayPower(bool) {}
void displayClear() {}

#endif
