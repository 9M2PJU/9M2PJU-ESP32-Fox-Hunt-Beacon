#include "display.h"

#if DISPLAY_TYPE == 1

// --- OLED SSD1306 via U8g2 ---
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);
static bool displayOn = true;

void displayInit(const char *callsign, const char *version) {
  Wire.begin(OLED_SDA, OLED_SCL);
  u8g2.begin();
  displayStartupScreen(callsign, version);
}

void displayStartupScreen(const char *callsign, const char *version) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_8x13B_tr);
  u8g2.setCursor(0, 14);
  u8g2.print(callsign);
  u8g2.print(" Fox");
  u8g2.drawLine(0, 18, 127, 18);
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
  u8g2.setFont(u8g2_font_8x13B_tr);
  u8g2.setCursor(0, 12);
  u8g2.print(callsign);
  u8g2.print(" ");
  u8g2.print(foxId);

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.setCursor(0, 26);
  u8g2.print(modeStr);
  u8g2.setCursor(0, 38);
  u8g2.print(stateStr);
  u8g2.setCursor(0, 50);
  u8g2.print(timingStr);
  u8g2.setCursor(0, 62);
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
  u8g2.setFont(u8g2_font_6x10_tr);

  // Header
  u8g2.setFont(u8g2_font_8x13B_tr);
  u8g2.drawStr(0, 12, "Settings");
  u8g2.drawLine(0, 15, 127, 15);
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
      u8g2.drawStr(0, y, ">");
    }
    char line[24];
    snprintf(line, sizeof(line), "%-10s %s", labels[idx], values[idx]);
    u8g2.drawStr(7, y, line);
  }

  // Footer hint
  u8g2.drawStr(0, 63, "1=Next 2=Toggle Hold=Exit");
  u8g2.sendBuffer();
}

void displayAPMode(const char *ssid, const char *ip) {
  if (!displayOn) return;
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_8x13B_tr);
  u8g2.drawStr(0, 12, "AP MODE");
  u8g2.drawLine(0, 15, 127, 15);
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

// --- TFT ST7789 via TFT_eSPI ---
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();
static bool displayOn = true;

void displayInit(const char *callsign, const char *version) {
  tft.init();
  tft.setRotation(1);
  displayStartupScreen(callsign, version);
}

void displayStartupScreen(const char *callsign, const char *version) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  String header = String(callsign) + " Fox";
  tft.drawString(header, 0, 0, 4);
  tft.drawLine(0, 32, tft.width(), 32, TFT_CYAN);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String ver = "v" + String(version);
  tft.drawString(ver, 0, 40, 2);
  tft.drawString("Starting...", 0, 60, 2);
}

void displayUpdate(const char *callsign, const char *foxId,
                   const char *modeStr, const char *stateStr,
                   const char *timingStr, const char *batteryStr,
                   const char *ipStr) {
  if (!displayOn) return;
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(callsign, 0, 0, 4);
  tft.drawString(foxId, 80, 0, 4);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(modeStr, 0, 30, 2);
  tft.drawString(stateStr, 0, 50, 2);
  tft.drawString(timingStr, 0, 70, 2);
  if (batteryStr[0]) {
    tft.drawString(batteryStr, 0, 90, 2);
  }
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(ipStr, 0, 110, 2);
}

void displayMenu(int selectedIndex, int itemCount,
                 const char *const labels[], const char *const values[]) {
  if (!displayOn) return;
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Settings", 0, 0, 4);
  tft.drawLine(0, 32, tft.width(), 32, TFT_CYAN);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);

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
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.drawString(">", 0, y, 2);
    } else {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
    }
    char line[32];
    snprintf(line, sizeof(line), "%-12s %s", labels[idx], values[idx]);
    tft.drawString(line, 14, y, 2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
  }

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("1=Next 2=Toggle Hold=Exit", 0, tft.height() - 16, 1);
}

void displayAPMode(const char *ssid, const char *ip) {
  if (!displayOn) return;
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("AP MODE", 0, 0, 4);
  tft.drawLine(0, 32, tft.width(), 32, TFT_YELLOW);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("SSID:", 0, 40, 2);
  tft.drawString(ssid, 0, 60, 2);
  tft.drawString(ip, 0, 80, 2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Web admin ready", 0, 100, 2);
}

void displayPower(bool on) {
  displayOn = on;
  if (on) {
    tft.fillScreen(TFT_BLACK);
  }
  // TFT_eSPI doesn't have a simple power-off; we just stop drawing.
  // Backlight control would need a BL pin PWM which varies per board.
}

void displayClear() {
  tft.fillScreen(TFT_BLACK);
}

#elif DISPLAY_TYPE == 3

// --- E-Ink SSD1680 via GxEPD2 ---
#include <GxEPD2_BW.h>
#include <SPI.h>

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
  eink.setCursor(0, 0);
  eink.print(callsign);
  eink.print(" Fox");
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
  eink.setCursor(0, 0);
  eink.print(callsign);
  eink.print(" ");
  eink.print(foxId);
  eink.setCursor(0, 16);
  eink.print(modeStr);
  eink.setCursor(0, 28);
  eink.print(stateStr);
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
  eink.setCursor(0, 0);
  eink.print("Settings");
  eink.setCursor(0, 12);
  eink.print("----------------");

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
      eink.setCursor(0, y);
      eink.print(">");
    }
    eink.setCursor(7, y);
    eink.print(labels[idx]);
    eink.print(" ");
    eink.print(values[idx]);
  }

  eink.setCursor(0, EINK_HEIGHT - 8);
  eink.print("1=Next 2=Toggle Hold=Exit");
  einkRefresh();
}

void displayAPMode(const char *ssid, const char *ip) {
  if (!displayOn) return;
  eink.setFullWindow();
  eink.fillScreen(GxEPD_WHITE);
  eink.setCursor(0, 0);
  eink.print("AP MODE");
  eink.setCursor(0, 12);
  eink.print("----------------");
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
