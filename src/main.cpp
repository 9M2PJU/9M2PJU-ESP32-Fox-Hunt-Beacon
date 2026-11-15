#include <Arduino.h>
#include <Preferences.h>
#include <OneButton.h>
#include <string.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <driver/rtc_io.h>
#include "beacon_config.h"
#include "display.h"
#include "web_admin.h"

#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= 0x030000)
#define FOX_USE_NEW_LEDC 1
#endif

constexpr uint8_t AUDIO_CHANNEL = 0;
constexpr uint8_t AUDIO_RESOLUTION_BITS = 10;
constexpr uint16_t AUDIO_DUTY = 512;
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t LED_IDLE_BLINK_MS = 1800;
constexpr uint32_t LED_TX_BLINK_MS = 160;
constexpr uint32_t LED_LOW_BATTERY_BLINK_MS = 350;
constexpr uint32_t PTT_TEST_MS = 1200;
constexpr uint32_t STARTUP_SCREEN_MS = 3000;
constexpr uint32_t MENU_TIMEOUT_MS = 30000;
constexpr uint32_t DISPLAY_ECO_TIMEOUT_MS = 4000;
constexpr uint32_t DISPLAY_ECO_GRACE_MS = 10000;  // no eco sleep for first 10s after boot
constexpr uint32_t TX_POLL_SLICE_MS = 10;  // cooperative slice inside blocking TX delays

Preferences preferences;
BeaconConfig config;
BeaconState state = BeaconState::StartupDelay;
uint32_t stateStartedAt = 0;
uint32_t lastLedToggleAt = 0;
bool ledOn = false;
bool forceTransmit = false;
String serialLine;
uint32_t lastDisplayUpdateAt = 0;

// Deferred requests from the async web task. Only flags + POD cross tasks;
// the main loop performs all hardware and flash work.
static portMUX_TYPE webMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool webTestRequested = false;
static volatile bool webPttRequested = false;
static volatile bool webDefaultsRequested = false;
static volatile bool webRebootRequested = false;
static volatile bool webConfigPending = false;
static PendingConfig pendingConfig;
static WebSnapshot webSnap;

// Menu-driven changes that must wait until TX is idle (WiFi + state flip).
static volatile bool menuApPending = false;
static volatile bool menuModePending = false;

// Battery cache + low-battery debounce.
static float cachedBatteryVoltage = 0.0f;
static uint32_t lastBatterySampleAt = 0;
static uint8_t lowBatteryTripStreak = 0;

// Delayed flash save: mark dirty on edits, flush after quiet period.
static bool configDirty = false;
static uint32_t configDirtyAt = 0;

// Boot fault guard: latched when consecutive panic/WDT/brownout boots pile
// up. While latched the radio stays off until the user clears it.
static bool faultHalt = false;

// Forward declarations used by the cooperative TX path.
void readSerialCommands();
void checkButton();
void serviceWebRequests();
bool canQueueTestTx();
void markConfigDirty();
void flushConfigIfDirty(bool force);
void recordBootFault();
void clearFaultHalt(const char *why);
uint8_t foxNumberFromId(const char *id);
uint8_t foxNumberFromId(const String &id);

// Menu state machine
enum class DisplayMode {
  StartupScreen,
  Status,
  Menu,
};
DisplayMode displayMode = DisplayMode::StartupScreen;
uint32_t displayModeEnteredAt = 0;
int menuIndex = 0;
uint32_t lastActivityAt = 0;
bool displaySleeping = false;

// OneButton instance for debounced multi-click detection.
#ifdef BUTTON_PIN
OneButton button(BUTTON_PIN, true);  // active LOW with internal pull-up
#endif

// Menu items: on/off toggles that can be changed from the display menu.
// Full settings (callsign, fox ID, timing, CW, PTT, etc.) require the web UI.
enum MenuItem {
  MENU_WIFI_AP = 0,
  MENU_WARBLE,
  MENU_FOX_SYNC,
  MENU_BATTERY,
  MENU_BEACON_MODE,
  MENU_ECO_MODE,
  MENU_EXIT,
  MENU_COUNT
};
const char *const menuLabels[] = {
  "WiFi AP",
  "Warble",
  "Fox Sync",
  "Battery",
  "Mode",
  "Eco Disp",
  "Exit",
};

int beaconStateValue() {
  return static_cast<int>(state);
}

float readBatteryVoltage();
uint8_t readBatteryPercent();
void saveConfig();
void loadDefaultConfig();
void enterState(BeaconState nextState);
uint32_t elapsedSince(uint32_t startedAt);

// Deferred web request entry points. Called from the async web task; they only
// set flags or copy POD, never touch hardware, display, or flash.
void queueWebTest() {
  portENTER_CRITICAL(&webMux);
  webTestRequested = true;
  portEXIT_CRITICAL(&webMux);
}
void queueWebPtt() {
  portENTER_CRITICAL(&webMux);
  webPttRequested = true;
  portEXIT_CRITICAL(&webMux);
}
void queueWebDefaults() {
  portENTER_CRITICAL(&webMux);
  webDefaultsRequested = true;
  portEXIT_CRITICAL(&webMux);
}
void queueWebReboot() {
  portENTER_CRITICAL(&webMux);
  webRebootRequested = true;
  portEXIT_CRITICAL(&webMux);
}
void queueWebConfig(const PendingConfig &cfg) {
  portENTER_CRITICAL(&webMux);
  pendingConfig = cfg;
  webConfigPending = true;
  portEXIT_CRITICAL(&webMux);
}

// True when 5-fox round-robin slots must overlap with current timing.
bool foxSlotsOverlap() {
  if (!config.foxSyncEnabled || config.beaconMode) return false;
  if (foxNumberFromId(config.foxId) == 0) return false;
  uint32_t cycle = config.transmitSeconds + config.idleSeconds;
  return (uint32_t)5 * config.transmitSeconds > cycle;
}

void petWatchdog() {
  esp_task_wdt_reset();
}

// Copy main-loop state into the web snapshot under lock. Called from the
// main task only. The async web task reads webSnap, never config.
void updateWebSnapshot();

// Reader for the async web task: copy snapshot out under lock.
void readWebSnapshot(WebSnapshot *dst) {
  portENTER_CRITICAL(&webMux);
  *dst = webSnap;
  portEXIT_CRITICAL(&webMux);
}

// Get the current value string for a menu item.
const char *menuValueStr(int idx) {
  switch (idx) {
    case MENU_WIFI_AP:    return config.wifiApEnabled ? "ON" : "OFF";
    case MENU_WARBLE:     return config.warbleEnabled ? "ON" : "OFF";
    case MENU_FOX_SYNC:   return config.foxSyncEnabled ? "ON" : "OFF";
    case MENU_BATTERY:    return config.batteryEnabled ? "ON" : "OFF";
    case MENU_BEACON_MODE:return config.beaconMode ? "BEACON" : "FOX";
    case MENU_ECO_MODE:   return config.displayEcoMode ? "ON" : "OFF";
    case MENU_EXIT:       return "";
    default:              return "";
  }
}

// Toggle the selected menu item and save config. WiFi AP and mode changes
// are deferred until TX is idle to avoid touching WiFi/RF mid-transmission.
void menuToggle(int idx) {
  switch (idx) {
    case MENU_WIFI_AP:
      config.wifiApEnabled = !config.wifiApEnabled;
      menuApPending = true;
      break;
    case MENU_WARBLE:
      config.warbleEnabled = !config.warbleEnabled;
      break;
    case MENU_FOX_SYNC:
      config.foxSyncEnabled = !config.foxSyncEnabled;
      break;
    case MENU_BATTERY:
      config.batteryEnabled = !config.batteryEnabled;
      if (config.batteryEnabled) {
        analogSetPinAttenuation(BATTERY_PIN, ADC_11db);
      }
      break;
    case MENU_BEACON_MODE:
      config.beaconMode = !config.beaconMode;
      menuModePending = true;
      break;
    case MENU_ECO_MODE:
      config.displayEcoMode = !config.displayEcoMode;
      break;
    case MENU_EXIT:
      displayMode = DisplayMode::Status;
      displayModeEnteredAt = millis();
      return;
  }
  markConfigDirty();
}

void renderMenu() {
  const char *values[MENU_COUNT];
  for (int i = 0; i < MENU_COUNT; i++) {
    values[i] = menuValueStr(i);
  }
  displayMenu(menuIndex, MENU_COUNT, menuLabels, values);
}

const char *stateToString(BeaconState s) {
  switch (s) {
    case BeaconState::StartupDelay: return "STARTUP";
    case BeaconState::Idle: return "IDLE";
    case BeaconState::Transmitting: return "TX";
    case BeaconState::ContinuousTransmit: return "BEACON";
    case BeaconState::LowBatteryHalt: return "LOWBAT";
    default: return "?";
  }
}

void updateDisplay() {
  // Handle startup screen transition
  if (displayMode == DisplayMode::StartupScreen) {
    if (elapsedSince(displayModeEnteredAt) >= STARTUP_SCREEN_MS) {
      displayMode = DisplayMode::Status;
      displayModeEnteredAt = millis();
      lastActivityAt = millis();
    }
    return;
  }

  // Throttle display updates to 1 per second for status, faster for menu
  if (displayMode == DisplayMode::Status && millis() - lastDisplayUpdateAt < 1000) return;
#if DISPLAY_TYPE == 3
  // E-Ink: refresh on content change, plus a slow heartbeat. Avoids ghosting
  // and wasted energy during long IDLE periods.
  {
    static uint32_t lastEinkAt = 0;
    static int lastEinkState = -1;
    static int lastEinkMode = -1;
    static char lastEinkBatt[20] = {0};
    char battNow[20] = {0};
    if (config.batteryEnabled) {
      snprintf(battNow, sizeof(battNow), "%.2fV", readBatteryVoltage());
    }
    int curState = static_cast<int>(state);
    int curMode = config.beaconMode ? 1 : 0;
    bool changed = (curState != lastEinkState) || (curMode != lastEinkMode) ||
                   (strcmp(battNow, lastEinkBatt) != 0);
    uint32_t now = millis();
    if (!changed && (now - lastEinkAt < EINK_HEARTBEAT_MS)) return;
    if (!changed) {
      // Heartbeat refresh still respects a minimum gap.
      if (now - lastEinkAt < EINK_MIN_UPDATE_MS) return;
    } else {
      if (now - lastEinkAt < EINK_MIN_UPDATE_MS && lastEinkState != -1) return;
    }
    lastEinkAt = now;
    lastEinkState = curState;
    lastEinkMode = curMode;
    strncpy(lastEinkBatt, battNow, sizeof(lastEinkBatt) - 1);
  }
#endif
  lastDisplayUpdateAt = millis();

  // Eco mode: sleep display after inactivity (with startup grace period)
  if (displayMode == DisplayMode::Status && config.displayEcoMode) {
    if (millis() < DISPLAY_ECO_GRACE_MS) return;  // grace period after boot
    if (!displaySleeping && elapsedSince(lastActivityAt) > DISPLAY_ECO_TIMEOUT_MS) {
      displaySleeping = true;
      displayPower(false);
      return;
    }
    if (displaySleeping) return;
  } else if (displaySleeping) {
    displaySleeping = false;
    displayPower(true);
  }

  // Menu timeout: return to status after inactivity
  if (displayMode == DisplayMode::Menu) {
    if (elapsedSince(lastActivityAt) > MENU_TIMEOUT_MS) {
      displayMode = DisplayMode::Status;
      displayModeEnteredAt = millis();
    } else {
      renderMenu();
      return;
    }
  }

  // Status screen
  char timingStr[32];
  snprintf(timingStr, sizeof(timingStr), "TX:%lus IDLE:%lus",
           static_cast<unsigned long>(config.transmitSeconds),
           static_cast<unsigned long>(config.idleSeconds));

  char batteryStr[20] = "";
  if (config.batteryEnabled) {
    snprintf(batteryStr, sizeof(batteryStr), "%.2fV %u%%",
             readBatteryVoltage(), readBatteryPercent());
  }

  displayUpdate(config.callSign,
                config.foxId,
                config.beaconMode ? "BEACON" : "FOX",
                stateToString(state),
                timingStr,
                batteryStr,
                webAdminIsRunning() ? webAdminGetIp().c_str() : "");
}

String normalizeId(String value) {
  value.trim();
  value.toUpperCase();
  String result;
  result.reserve(value.length());
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value.charAt(i);
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '/' || c == '-') {
      result += c;
    }
  }
  return result.length() ? result : "FOX";
}

// Fixed-buffer variant: no heap growth in the live config. Keeps A-Z 0-9 / -,
// uppercases, truncates, falls back to FOX.
void normalizeIdTo(const char *src, char *dst, size_t dstSize) {
  if (dstSize == 0) return;
  size_t w = 0;
  if (src) {
    for (size_t i = 0; src[i] != '\0' && w + 1 < dstSize; ++i) {
      char c = src[i];
      if (c >= 'a' && c <= 'z') c -= 32;
      if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '/' || c == '-') {
        dst[w++] = c;
      }
    }
  }
  if (w == 0 && dstSize > 3) {
    dst[0] = 'F'; dst[1] = 'O'; dst[2] = 'X'; w = 3;
  }
  dst[w] = '\0';
}

void setCallSign(const char *v) { normalizeIdTo(v, config.callSign, sizeof(config.callSign)); }
void setFoxId(const char *v) { normalizeIdTo(v, config.foxId, sizeof(config.foxId)); }
void setWifiPassword(const char *v) {
  if (!v || !v[0]) { config.wifiApPassword[0] = '\0'; return; }
  // Keep printable ASCII only so corrupt NVS can never break softAP().
  size_t w = 0;
  for (size_t i = 0; v[i] != '\0' && w < sizeof(config.wifiApPassword) - 1; ++i) {
    unsigned char c = (unsigned char)v[i];
    if (c >= 32 && c <= 126) config.wifiApPassword[w++] = (char)c;
  }
  config.wifiApPassword[w] = '\0';
}

// Returns the ARDF fox number (1-5) for a standard fox ID, or 0 if the ID is
// not one of the five standard IARU identifiers.
uint8_t foxNumberFromId(const char *id) {
  char norm[16];
  normalizeIdTo(id, norm, sizeof(norm));
  if (strcmp(norm, "MOE") == 0) return 1;
  if (strcmp(norm, "MOI") == 0) return 2;
  if (strcmp(norm, "MOS") == 0) return 3;
  if (strcmp(norm, "MOH") == 0) return 4;
  if (strcmp(norm, "MO5") == 0) return 5;
  return 0;
}
uint8_t foxNumberFromId(const String &id) { return foxNumberFromId(id.c_str()); }

// When fox-slot sync is enabled and the fox ID is a standard ARDF identifier,
// the startup delay is overridden so that five beacons powered on at roughly
// the same time fall into the standard round-robin order. Fox 1 starts
// immediately, fox 2 after one TX slot, and so on.
uint32_t resolvedStartupDelaySeconds() {
  if (!config.foxSyncEnabled || config.beaconMode) {
    return config.startupDelaySeconds;
  }
  const uint8_t foxNumber = foxNumberFromId(config.foxId);
  if (foxNumber == 0) {
    return config.startupDelaySeconds;
  }
  return static_cast<uint32_t>(foxNumber - 1) * config.transmitSeconds;
}

uint32_t elapsedSince(uint32_t startedAt) {
  return millis() - startedAt;
}

void setLed(bool on) {
  ledOn = on;
  digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
}

void blinkLed(uint32_t intervalMs) {
  if (elapsedSince(lastLedToggleAt) >= intervalMs) {
    lastLedToggleAt = millis();
    setLed(!ledOn);
  }
}

void audioInit() {
#ifdef FOX_USE_NEW_LEDC
  ledcAttach(AUDIO_PIN, config.cwToneHz, AUDIO_RESOLUTION_BITS);
#else
  ledcSetup(AUDIO_CHANNEL, config.cwToneHz, AUDIO_RESOLUTION_BITS);
  ledcAttachPin(AUDIO_PIN, AUDIO_CHANNEL);
#endif
}

void audioOff() {
#ifdef FOX_USE_NEW_LEDC
  ledcWriteTone(AUDIO_PIN, 0);
  ledcWrite(AUDIO_PIN, 0);
#else
  ledcWriteTone(AUDIO_CHANNEL, 0);
  ledcWrite(AUDIO_CHANNEL, 0);
#endif
}

void audioTone(uint16_t frequencyHz) {
#ifdef FOX_USE_NEW_LEDC
  ledcWriteTone(AUDIO_PIN, frequencyHz);
  ledcWrite(AUDIO_PIN, AUDIO_DUTY);
#else
  ledcWriteTone(AUDIO_CHANNEL, frequencyHz);
  ledcWrite(AUDIO_CHANNEL, AUDIO_DUTY);
#endif
}

void setPtt(bool active) {
  const bool outputHigh = config.pttActiveLow ? !active : active;
  digitalWrite(PTT_PIN, outputHigh ? HIGH : LOW);
}

// RTC hold keeps PTT at its safe level across software resets and the
// bootloader window, where the pin would otherwise float. No-op on pins
// without RTC hold support - those boards need an external pull-down.
static bool pttHoldActive = false;

static int pttSafeGpioLevel() {
  return config.pttActiveLow ? HIGH : LOW;
}

static void pttHoldSafe() {
  if (!rtc_gpio_is_valid_gpio((gpio_num_t)PTT_PIN)) return;
  rtc_gpio_set_level((gpio_num_t)PTT_PIN, (uint32_t)pttSafeGpioLevel());
  if (rtc_gpio_hold_en((gpio_num_t)PTT_PIN) == ESP_OK) pttHoldActive = true;
}

static void pttHoldRelease() {
  if (!pttHoldActive) return;
  rtc_gpio_hold_dis((gpio_num_t)PTT_PIN);
  pttHoldActive = false;
}

void safeRadioOff() {
  audioOff();
  setPtt(false);
  setLed(false);
  pttHoldSafe();
}

static float sampleBatteryRaw() {
  constexpr uint8_t samples = 12;
  uint32_t readings[12];
  for (uint8_t i = 0; i < samples; ++i) {
    readings[i] = analogReadMilliVolts(BATTERY_PIN);
    delay(2);
  }
  // Insertion sort + median to reject spikes.
  for (uint8_t i = 1; i < samples; ++i) {
    uint32_t key = readings[i];
    int8_t j = i - 1;
    while (j >= 0 && readings[j] > key) {
      readings[j + 1] = readings[j];
      --j;
    }
    readings[j + 1] = key;
  }
  const uint32_t medianMv = (readings[5] + readings[6]) / 2;
  return (medianMv / 1000.0f) * config.batteryScale;
}

float readBatteryVoltage() {
  if (!config.batteryEnabled) {
    return 99.0f;
  }
  uint32_t now = millis();
  if (lastBatterySampleAt != 0 && (now - lastBatterySampleAt) < BATTERY_SAMPLE_MS &&
      cachedBatteryVoltage > 0.0f) {
    return cachedBatteryVoltage;
  }
  cachedBatteryVoltage = sampleBatteryRaw();
  lastBatterySampleAt = now;
  return cachedBatteryVoltage;
}

float readBatteryVoltageFresh() {
  if (!config.batteryEnabled) return 99.0f;
  cachedBatteryVoltage = sampleBatteryRaw();
  lastBatterySampleAt = millis();
  return cachedBatteryVoltage;
}

// Estimate state-of-charge (%) for a single-cell Li-ion battery from its
// resting open-circuit voltage. The curve is a piecewise-linear fit to a
// typical 18650 / LiPo discharge profile. For multi-cell packs the scaled
// voltage must fall in the single-cell range (3.2–4.2 V); otherwise the
// percentage is clamped to 0 or 100.
//
// Reference points (V -> %):
//   4.20=100  4.10=90  4.00=80  3.90=70  3.80=55
//   3.70=40   3.60=24  3.50=12  3.40=5   3.30=1  3.20=0
uint8_t readBatteryPercent() {
  if (!config.batteryEnabled) return 0;
  const float v = readBatteryVoltage();

  static const struct { float v; uint8_t pct; } pts[] = {
    {4.20f, 100}, {4.10f, 90}, {4.00f, 80}, {3.90f, 70},
    {3.80f, 55},  {3.70f, 40}, {3.60f, 24}, {3.50f, 12},
    {3.40f, 5},   {3.30f, 1},  {3.20f, 0},
  };
  constexpr uint8_t n = sizeof(pts) / sizeof(pts[0]);

  if (v >= pts[0].v) return 100;
  if (v <= pts[n - 1].v) return 0;

  for (uint8_t i = 1; i < n; ++i) {
    if (v >= pts[i].v) {
      const float dv = pts[i - 1].v - pts[i].v;
      const float dp = pts[i - 1].pct - pts[i].pct;
      return pts[i].pct + static_cast<uint8_t>((v - pts[i].v) / dv * dp + 0.5f);
    }
  }
  return 0;
}

bool isLowBatteryCached() {
  if (!config.batteryEnabled) return false;
  if (cachedBatteryVoltage <= 0.0f) return false;
  return cachedBatteryVoltage <= config.lowBatteryVoltage;
}

bool isLowBattery() {
  if (!config.batteryEnabled) { lowBatteryTripStreak = 0; return false; }
  // Ignore key-up sag: TX load dips the rail for a few seconds after PTT on.
  if ((state == BeaconState::Transmitting || state == BeaconState::ContinuousTransmit) &&
      elapsedSince(stateStartedAt) < TX_BATTERY_GRACE_MS) {
    return false;
  }
  // During TX use the pre-TX cached reading so the 24ms ADC block does not
  // jitter CW timing. Fresh sampling happens only with the radio off.
  if (state == BeaconState::Transmitting || state == BeaconState::ContinuousTransmit) {
    if (isLowBatteryCached()) {
      if (++lowBatteryTripStreak >= LOW_BATTERY_TRIP_COUNT) return true;
    } else {
      lowBatteryTripStreak = 0;
    }
    return false;
  }
  // Use a fresh sample for halt decisions; the cached value is for display.
  const float v = readBatteryVoltageFresh();
  if (v <= config.lowBatteryVoltage) {
    if (++lowBatteryTripStreak >= LOW_BATTERY_TRIP_COUNT) return true;
    return (state == BeaconState::LowBatteryHalt);
  }
  lowBatteryTripStreak = 0;
  if (state == BeaconState::LowBatteryHalt) {
    // Require recovery above cutoff + hysteresis before resuming.
    return v < (config.lowBatteryVoltage + LOW_BATTERY_HYSTERESIS_V);
  }
  return false;
}

bool isBatteryRecovered() {
  if (!config.batteryEnabled) return true;
  return readBatteryVoltageFresh() >= (config.lowBatteryVoltage + LOW_BATTERY_HYSTERESIS_V);
}

// Cooperative sleep used inside TX. Services serial, button, and web tasks in
// small slices so a 60s transmission does not freeze the firmware. Returns
// false if TX must abort (low battery or reboot requested).
bool keyedDelayAbortable(uint32_t durationMs) {
  const uint32_t startedAt = millis();
  while (elapsedSince(startedAt) < durationMs) {
    uint32_t remaining = durationMs - elapsedSince(startedAt);
    uint32_t slice = remaining < TX_POLL_SLICE_MS ? remaining : TX_POLL_SLICE_MS;
    delay(slice);
    petWatchdog();
    readSerialCommands();
    checkButton();
    webAdminLoop();
    if (webRebootRequested) return false;
    // Cheap mid-TX battery guard without re-sampling ADC every 10ms:
    // check cached value, full fresh check happens in transmit loops.
    if (state == BeaconState::LowBatteryHalt) return false;
  }
  return true;
}

void keyedDelay(uint32_t durationMs) {
  (void)keyedDelayAbortable(durationMs);
}

const char *morseFor(char c) {
  switch (c) {
    case 'A': return ".-";
    case 'B': return "-...";
    case 'C': return "-.-.";
    case 'D': return "-..";
    case 'E': return ".";
    case 'F': return "..-.";
    case 'G': return "--.";
    case 'H': return "....";
    case 'I': return "..";
    case 'J': return ".---";
    case 'K': return "-.-";
    case 'L': return ".-..";
    case 'M': return "--";
    case 'N': return "-.";
    case 'O': return "---";
    case 'P': return ".--.";
    case 'Q': return "--.-";
    case 'R': return ".-.";
    case 'S': return "...";
    case 'T': return "-";
    case 'U': return "..-";
    case 'V': return "...-";
    case 'W': return ".--";
    case 'X': return "-..-";
    case 'Y': return "-.--";
    case 'Z': return "--..";
    case '0': return "-----";
    case '1': return ".----";
    case '2': return "..---";
    case '3': return "...--";
    case '4': return "....-";
    case '5': return ".....";
    case '6': return "-....";
    case '7': return "--...";
    case '8': return "---..";
    case '9': return "----.";
    case '/': return "-..-.";
    case '-': return "-....-";
    default: return "";
  }
}

bool sendMorseTextAbortable(const char *text, uint16_t wpm, uint16_t toneHz) {
  const uint32_t dotMs = 1200UL / max<uint16_t>(wpm, 5);
  char id[16];
  normalizeIdTo(text, id, sizeof(id));

  for (size_t charIndex = 0; id[charIndex] != '\0'; ++charIndex) {
    const char c = id[charIndex];
    if (c == ' ') {
      if (!keyedDelayAbortable(dotMs * 7)) return false;
      continue;
    }

    const char *pattern = morseFor(c);
    for (size_t i = 0; pattern[i] != '\0'; ++i) {
      audioTone(toneHz);
      if (!keyedDelayAbortable(pattern[i] == '-' ? dotMs * 3 : dotMs)) return false;
      audioOff();
      if (!keyedDelayAbortable(dotMs)) return false;
      if (isLowBatteryCached()) return false;
    }

    if (!keyedDelayAbortable(dotMs * 2)) return false;
    if (isLowBatteryCached()) return false;
  }
  return true;
}

void sendMorseText(const String &text) {
  (void)sendMorseTextAbortable(text.c_str(), config.cwWpm, config.cwToneHz);
}

bool sendWarbleUntilAbortable(uint32_t untilMs, uint16_t lowHz, uint16_t highHz,
                              uint16_t stepMs) {
  bool high = false;
  while (static_cast<int32_t>(untilMs - millis()) > 0) {
    if (isLowBatteryCached() || webRebootRequested) return false;
    audioTone(high ? highHz : lowHz);
    high = !high;
    int32_t left = static_cast<int32_t>(untilMs - millis());
    if (left <= 0) break;
    const uint32_t step = min<uint32_t>(stepMs, static_cast<uint32_t>(left));
    if (!keyedDelayAbortable(step)) return false;
  }
  audioOff();
  return true;
}

void sendWarbleUntil(uint32_t untilMs) {
  (void)sendWarbleUntilAbortable(untilMs, config.warbleLowHz, config.warbleHighHz,
                                 config.warbleStepMs);
}

void saveConfig() {
  config.callSign[sizeof(config.callSign) - 1] = '\0';
  config.foxId[sizeof(config.foxId) - 1] = '\0';
  config.wifiApPassword[sizeof(config.wifiApPassword) - 1] = '\0';
  preferences.begin("foxbeacon", false);
  preferences.putString("call", config.callSign);
  preferences.putString("fox", config.foxId);
  preferences.putUInt("startup", config.startupDelaySeconds);
  preferences.putUInt("tx", config.transmitSeconds);
  preferences.putUInt("idle", config.idleSeconds);
  preferences.putUShort("wpm", config.cwWpm);
  preferences.putUShort("tone", config.cwToneHz);
  preferences.putUShort("warbleLo", config.warbleLowHz);
  preferences.putUShort("warbleHi", config.warbleHighHz);
  preferences.putUShort("warbleMs", config.warbleStepMs);
  preferences.putUShort("lead", config.pttLeadMs);
  preferences.putUShort("tail", config.pttTailMs);
  preferences.putBool("pttLow", config.pttActiveLow);
  preferences.putBool("warble", config.warbleEnabled);
  preferences.putBool("batEn", config.batteryEnabled);
  preferences.putFloat("batScale", config.batteryScale);
  preferences.putFloat("batLow", config.lowBatteryVoltage);
  preferences.putBool("foxSync", config.foxSyncEnabled);
  preferences.putBool("beaconMode", config.beaconMode);
  preferences.putUShort("beaconId", config.beaconIdIntervalSeconds);
  preferences.putBool("wifiAp", config.wifiApEnabled);
  preferences.putUShort("wifiApTo", config.wifiApTimeoutMinutes);
  preferences.putBool("ecoDisp", config.displayEcoMode);
  preferences.putString("wifiPw", config.wifiApPassword);
  preferences.putUShort("cfgVer", CONFIG_VERSION);
  preferences.end();
  configDirty = false;
  updateWebSnapshot();
}

void markConfigDirty() {
  configDirty = true;
  configDirtyAt = millis();
  updateWebSnapshot();
}

void flushConfigIfDirty(bool force) {
  if (!configDirty) return;
  if (!force && millis() - configDirtyAt < CONFIG_SAVE_DELAY_MS) return;
  if (!force && (state == BeaconState::Transmitting || state == BeaconState::ContinuousTransmit)) return;
  saveConfig();
}

void loadDefaultConfig() {
  config = BeaconConfig();
  char tmp[16];
  normalizeIdTo(config.callSign, tmp, sizeof(tmp));
  strncpy(config.callSign, tmp, sizeof(config.callSign) - 1);
  normalizeIdTo(config.foxId, tmp, sizeof(tmp));
  strncpy(config.foxId, tmp, sizeof(config.foxId) - 1);
}

void loadConfig() {
  loadDefaultConfig();

  preferences.begin("foxbeacon", true);
  {
    String v = preferences.getString("call", config.callSign);
    setCallSign(v.c_str());
    v = preferences.getString("fox", config.foxId);
    setFoxId(v.c_str());
  }
  config.startupDelaySeconds = preferences.getUInt("startup", config.startupDelaySeconds);
  config.transmitSeconds = preferences.getUInt("tx", config.transmitSeconds);
  config.idleSeconds = preferences.getUInt("idle", config.idleSeconds);
  config.cwWpm = preferences.getUShort("wpm", config.cwWpm);
  config.cwToneHz = preferences.getUShort("tone", config.cwToneHz);
  config.warbleLowHz = preferences.getUShort("warbleLo", config.warbleLowHz);
  config.warbleHighHz = preferences.getUShort("warbleHi", config.warbleHighHz);
  config.warbleStepMs = preferences.getUShort("warbleMs", config.warbleStepMs);
  config.pttLeadMs = preferences.getUShort("lead", config.pttLeadMs);
  config.pttTailMs = preferences.getUShort("tail", config.pttTailMs);
  config.pttActiveLow = preferences.getBool("pttLow", config.pttActiveLow);
  config.warbleEnabled = preferences.getBool("warble", config.warbleEnabled);
  config.batteryEnabled = preferences.getBool("batEn", config.batteryEnabled);
  config.batteryScale = preferences.getFloat("batScale", config.batteryScale);
  config.lowBatteryVoltage = preferences.getFloat("batLow", config.lowBatteryVoltage);
  config.foxSyncEnabled = preferences.getBool("foxSync", config.foxSyncEnabled);
  config.beaconMode = preferences.getBool("beaconMode", config.beaconMode);
  config.beaconIdIntervalSeconds = preferences.getUShort("beaconId", config.beaconIdIntervalSeconds);
  config.wifiApEnabled = preferences.getBool("wifiAp", config.wifiApEnabled);
  config.wifiApTimeoutMinutes = preferences.getUShort("wifiApTo", config.wifiApTimeoutMinutes);
  config.displayEcoMode = preferences.getBool("ecoDisp", config.displayEcoMode);
  {
    String v = preferences.getString("wifiPw", config.wifiApPassword);
    setWifiPassword(v.c_str());
  }
  (void)preferences.getUShort("cfgVer", 1);
  preferences.end();

  config.startupDelaySeconds = constrain(config.startupDelaySeconds, 0UL, 3600UL);
  config.transmitSeconds = constrain(config.transmitSeconds, 3UL, 600UL);
  config.idleSeconds = constrain(config.idleSeconds, 5UL, 3600UL);
  config.cwWpm = constrain(config.cwWpm, 5, 35);
  config.cwToneHz = constrain(config.cwToneHz, 300, 1800);
  config.warbleLowHz = constrain(config.warbleLowHz, 300, 1800);
  config.warbleHighHz = constrain(config.warbleHighHz, 300, 1800);
  config.warbleStepMs = constrain(config.warbleStepMs, 100, 3000);
  config.pttLeadMs = constrain(config.pttLeadMs, 50, 3000);
  config.pttTailMs = constrain(config.pttTailMs, 50, 3000);
  config.batteryScale = constrain(config.batteryScale, 1.0f, 10.0f);
  config.lowBatteryVoltage = constrain(config.lowBatteryVoltage, 2.5f, 15.0f);
  config.beaconIdIntervalSeconds = constrain(config.beaconIdIntervalSeconds, 10, 600);
  config.wifiApTimeoutMinutes = constrain(config.wifiApTimeoutMinutes, 0, 1440);
  config.wifiApPassword[sizeof(config.wifiApPassword) - 1] = '\0';
  configDirty = false;
}

void recordBootFault() {
  esp_reset_reason_t reason = esp_reset_reason();
  bool fault = (reason == ESP_RST_PANIC) || (reason == ESP_RST_INT_WDT) ||
               (reason == ESP_RST_TASK_WDT) || (reason == ESP_RST_WDT) ||
               (reason == ESP_RST_BROWNOUT);
  preferences.begin("foxboot", false);
  uint32_t boots = preferences.getUInt("boots", 0) + 1;
  preferences.putUInt("boots", boots);
  uint32_t streak = preferences.getUInt("faults", 0);
  streak = fault ? streak + 1 : 0;
  preferences.putUInt("faults", streak);
  preferences.putUInt("lastReason", (uint32_t)reason);
  preferences.end();
  Serial.printf("Boot #%lu reason %d fault streak %lu\n", (unsigned long)boots, (int)reason,
                (unsigned long)streak);
  if (streak >= BOOT_FAULT_TRIP_COUNT) {
    faultHalt = true;
    Serial.println(F("FAULT HALT: repeated panic/WDT/brownout. TX locked."));
    Serial.println(F("Check wiring and power, then hold button 2s or type clearfault."));
  }
}

void clearFaultHalt(const char *why) {
  preferences.begin("foxboot", false);
  preferences.putUInt("faults", 0);
  preferences.end();
  faultHalt = false;
  lowBatteryTripStreak = 0;
  Serial.printf("Fault halt cleared (%s). Resuming.\n", why);
  enterState(config.beaconMode ? BeaconState::ContinuousTransmit : BeaconState::Idle);
  updateWebSnapshot();
}

void updateWebSnapshot() {
  WebSnapshot s;
  strncpy(s.call, config.callSign, sizeof(s.call) - 1);
  strncpy(s.fox, config.foxId, sizeof(s.fox) - 1);
  s.beaconMode = config.beaconMode;
  s.foxSync = config.foxSyncEnabled;
  s.foxNum = foxNumberFromId(config.foxId);
  s.startup = config.startupDelaySeconds;
  s.startupResolved = resolvedStartupDelaySeconds();
  s.tx = config.transmitSeconds;
  s.idle = config.idleSeconds;
  s.beaconId = config.beaconIdIntervalSeconds;
  s.wpm = config.cwWpm;
  s.tone = config.cwToneHz;
  s.warble = config.warbleEnabled;
  s.warbleLow = config.warbleLowHz;
  s.warbleHigh = config.warbleHighHz;
  s.warbleStep = config.warbleStepMs;
  s.pttActiveLow = config.pttActiveLow;
  s.lead = config.pttLeadMs;
  s.tail = config.pttTailMs;
  s.batteryEnabled = config.batteryEnabled;
  s.batteryScale = config.batteryScale;
  s.lowBattery = config.lowBatteryVoltage;
  s.wifiAp = config.wifiApEnabled;
  s.wifiApTimeout = config.wifiApTimeoutMinutes;
  s.ecoMode = config.displayEcoMode;
  s.wifiPwSet = config.wifiApPassword[0] != '\0';
  s.overlap = foxSlotsOverlap();
  s.state = static_cast<int>(state);
  if (config.batteryEnabled) {
    snprintf(s.batteryStr, sizeof(s.batteryStr), "%.2f", readBatteryVoltage());
    snprintf(s.batteryPctStr, sizeof(s.batteryPctStr), "%u", readBatteryPercent());
    s.hasBatteryPct = true;
  } else {
    strncpy(s.batteryStr, "N/A", sizeof(s.batteryStr) - 1);
    s.hasBatteryPct = false;
  }
  extern String webAdminGetSsid();
  extern String webAdminGetIp();
  strncpy(s.apSsid, webAdminGetSsid().c_str(), sizeof(s.apSsid) - 1);
  strncpy(s.ip, webAdminGetIp().c_str(), sizeof(s.ip) - 1);
  portENTER_CRITICAL(&webMux);
  webSnap = s;
  portEXIT_CRITICAL(&webMux);
}

void printConfig() {
  Serial.println();
  Serial.println(F("ESP32 Fox Hunt Beacon"));
  Serial.println(F("---------------------"));
  Serial.printf("Callsign:       %s\n", config.callSign);
  Serial.printf("Fox ID:         %s\n", config.foxId);
  Serial.printf("Mode:           %s\n", config.beaconMode ? "beacon (continuous)" : "fox (scheduled)");
  Serial.printf("Startup delay:  %lu s\n", static_cast<unsigned long>(config.startupDelaySeconds));
  if (config.foxSyncEnabled && !config.beaconMode) {
    const uint8_t foxNumber = foxNumberFromId(config.foxId);
    if (foxNumber > 0) {
      Serial.printf("                (fox sync: slot %u, starts at %lu s)\n",
                    foxNumber,
                    static_cast<unsigned long>(resolvedStartupDelaySeconds()));
    } else {
      Serial.println(F("                (fox sync on but fox ID is not a standard ARDF ID)"));
    }
  }
  Serial.printf("Transmit time:  %lu s\n", static_cast<unsigned long>(config.transmitSeconds));
  Serial.printf("Idle time:      %lu s\n", static_cast<unsigned long>(config.idleSeconds));
  if (foxSlotsOverlap()) {
    Serial.println(F("WARNING: 5-fox slots overlap: 5*TX > TX+IDLE. Lengthen idle."));
  }
  if (config.beaconMode) {
    Serial.printf("Beacon ID every:%u s\n", config.beaconIdIntervalSeconds);
  }
  Serial.printf("CW:             %u WPM at %u Hz\n", config.cwWpm, config.cwToneHz);
  Serial.printf("Warble:         %s, %u/%u Hz, %u ms step\n",
                config.warbleEnabled ? "on" : "off",
                config.warbleLowHz,
                config.warbleHighHz,
                config.warbleStepMs);
  Serial.printf("PTT:            GPIO %u, active %s, lead %u ms, tail %u ms\n",
                PTT_PIN,
                config.pttActiveLow ? "LOW" : "HIGH",
                config.pttLeadMs,
                config.pttTailMs);
  Serial.printf("Audio:          GPIO %u\n", AUDIO_PIN);
  Serial.printf("LED/Button:     GPIO %u / GPIO %u\n", LED_PIN, BUTTON_PIN);
  Serial.printf("Battery:        %s, GPIO %u, scale %.3f, cutoff %.2f V",
                config.batteryEnabled ? "on" : "off",
                BATTERY_PIN,
                config.batteryScale,
                config.lowBatteryVoltage);
  if (config.batteryEnabled) {
    Serial.printf(", now %.2f V (%u%%)", readBatteryVoltage(), readBatteryPercent());
  }
  Serial.println();
  Serial.printf("WiFi AP:        %s", config.wifiApEnabled ? "on" : "off");
  if (config.wifiApEnabled && config.wifiApTimeoutMinutes > 0) {
    Serial.printf(" (auto-off %u min)", config.wifiApTimeoutMinutes);
  }
  Serial.printf(", password %s", config.wifiApPassword[0] ? "set" : "open");
  Serial.println();
  Serial.printf("Display eco:    %s\n", config.displayEcoMode ? "on" : "off");
  Serial.printf("Version:        %s\n", FIRMWARE_VERSION);
  Serial.println();
  Serial.println(F("Commands:"));
  Serial.println(F("  show"));
  Serial.println(F("  test"));
  Serial.println(F("  ptt_test"));
  Serial.println(F("  sync            (restart cycle clock for multi-fox alignment)"));
  Serial.println(F("  defaults"));
  Serial.println(F("  clearfault      (clear repeated crash/brownout halt)"));
  Serial.println(F("  reboot"));
  Serial.println(F("  set call 9M2PJU"));
  Serial.println(F("  set fox MOE"));
  Serial.println(F("  set mode fox|beacon"));
  Serial.println(F("  set fox_sync on|off"));
  Serial.println(F("  set startup 300"));
  Serial.println(F("  set tx 60"));
  Serial.println(F("  set idle 240"));
  Serial.println(F("  set beacon_id 60"));
  Serial.println(F("  set wpm 12"));
  Serial.println(F("  set tone 700"));
  Serial.println(F("  set warble on|off"));
  Serial.println(F("  set warble_low 700"));
  Serial.println(F("  set warble_high 900"));
  Serial.println(F("  set warble_step 350"));
  Serial.println(F("  set lead 350"));
  Serial.println(F("  set tail 350"));
  Serial.println(F("  set ptt active_low|active_high"));
  Serial.println(F("  set battery on|off"));
  Serial.println(F("  set battery_scale 2.0"));
  Serial.println(F("  set low_battery 3.4"));
  Serial.println(F("  set wifi_ap on|off"));
  Serial.println(F("  set wifi_ap_timeout 10  (0=never)"));
  Serial.println(F("  set wifi_password secret  (empty=open, 8-64 chars)"));
  Serial.println(F("  set eco_mode on|off"));
  Serial.println();
}

void enterState(BeaconState nextState) {
  state = nextState;
  stateStartedAt = millis();
  lastLedToggleAt = millis();
  updateWebSnapshot();
}

void transmitBeacon() {
  Serial.println(F("TX start"));
  forceTransmit = false;  // consume any test request that arrived mid-TX
  // Snapshot slot params so web/serial/menu edits mid-slot apply next slot.
  char slotCall[16];
  char slotFox[16];
  strncpy(slotCall, config.callSign, sizeof(slotCall) - 1);
  slotCall[sizeof(slotCall) - 1] = '\0';
  strncpy(slotFox, config.foxId, sizeof(slotFox) - 1);
  slotFox[sizeof(slotFox) - 1] = '\0';
  const uint32_t slotTx = config.transmitSeconds;
  const uint16_t slotWpm = config.cwWpm;
  const uint16_t slotTone = config.cwToneHz;
  const bool slotWarble = config.warbleEnabled;
  const uint16_t slotWarbleLow = config.warbleLowHz;
  const uint16_t slotWarbleHigh = config.warbleHighHz;
  const uint16_t slotWarbleStep = config.warbleStepMs;
  const uint16_t slotLead = config.pttLeadMs;
  const uint16_t slotTail = config.pttTailMs;
  if (config.batteryEnabled) readBatteryVoltageFresh();
  setLed(true);
  pttHoldRelease();
  setPtt(true);
  bool ok = keyedDelayAbortable(slotLead);
  const uint32_t transmissionEndsAt = millis() + (slotTx * 1000UL);

  if (ok) ok = sendMorseTextAbortable(slotCall, slotWpm, slotTone);
  if (ok) ok = keyedDelayAbortable(500);
  if (ok) ok = sendMorseTextAbortable(slotFox, slotWpm, slotTone);
  if (ok) ok = keyedDelayAbortable(500);

  if (ok && slotWarble && static_cast<int32_t>(transmissionEndsAt - millis()) > 0) {
    ok = sendWarbleUntilAbortable(transmissionEndsAt, slotWarbleLow, slotWarbleHigh,
                                  slotWarbleStep);
  } else {
    audioOff();
    if (ok && static_cast<int32_t>(transmissionEndsAt - millis()) > 0) {
      // IARU steady carrier: hold PTT with no audio until the slot ends,
      // polling so serial/button/web stay alive.
      uint32_t until = transmissionEndsAt;
      while (ok && static_cast<int32_t>(until - millis()) > 0) {
        if (isLowBatteryCached() || webRebootRequested) { ok = false; break; }
        blinkLed(LED_TX_BLINK_MS);
        if (!keyedDelayAbortable(50)) { ok = false; break; }
      }
    }
  }

  if (!ok) Serial.println(F("TX aborted"));
  (void)keyedDelayAbortable(slotTail);
  safeRadioOff();
  Serial.println(F("TX end"));
}

void readSerialCommands();

void runContinuousBeacon() {
  Serial.println(F("Continuous beacon start"));
  char slotCall[16];
  char slotFox[16];
  strncpy(slotCall, config.callSign, sizeof(slotCall) - 1);
  slotCall[sizeof(slotCall) - 1] = '\0';
  strncpy(slotFox, config.foxId, sizeof(slotFox) - 1);
  slotFox[sizeof(slotFox) - 1] = '\0';
  const uint16_t slotWpm = config.cwWpm;
  const uint16_t slotTone = config.cwToneHz;
  const uint16_t slotLead = config.pttLeadMs;
  const uint16_t slotTail = config.pttTailMs;
  const uint16_t slotBeaconId = config.beaconIdIntervalSeconds;
  if (config.batteryEnabled) readBatteryVoltageFresh();
  setLed(true);
  pttHoldRelease();
  setPtt(true);
  bool ok = keyedDelayAbortable(slotLead);

  while (ok && state == BeaconState::ContinuousTransmit) {
    audioOff();
    if (webRebootRequested) break;
    ok = sendMorseTextAbortable(slotCall, slotWpm, slotTone);
    if (!ok) break;
    ok = keyedDelayAbortable(500);
    if (!ok) break;
    ok = sendMorseTextAbortable(slotFox, slotWpm, slotTone);
    if (!ok) break;
    ok = keyedDelayAbortable(500);
    if (!ok) break;
    audioOff();

    const uint32_t carrierUntil = millis() + (slotBeaconId * 1000UL);
    while (ok && static_cast<int32_t>(carrierUntil - millis()) > 0) {
      if (isLowBatteryCached() || webRebootRequested) {
        ok = false;
        break;
      }
      blinkLed(LED_TX_BLINK_MS);
      if (!keyedDelayAbortable(50)) { ok = false; break; }
      serviceWebRequests();
      if (webRebootRequested) { ok = false; break; }
    }
    if (isLowBatteryCached()) break;
  }

  if (!ok) Serial.println(F("Continuous beacon aborted"));
  (void)keyedDelayAbortable(slotTail);
  safeRadioOff();
  Serial.println(F("Continuous beacon end"));
}

void testPttOnly() {
  Serial.println(F("PTT test start"));
  audioOff();
  pttHoldRelease();
  setPtt(true);
  setLed(true);
  (void)keyedDelayAbortable(PTT_TEST_MS);
  safeRadioOff();
  Serial.println(F("PTT test end"));
}

bool parseBoolValue(const String &value, bool *out) {
  if (value == "on" || value == "true" || value == "yes" || value == "1") {
    *out = true;
    return true;
  }
  if (value == "off" || value == "false" || value == "no" || value == "0") {
    *out = false;
    return true;
  }
  return false;
}

void handleSetCommand(String key, String value) {
  key.toLowerCase();
  value.trim();

  if (key == "call") {
    setCallSign(value.c_str());
  } else if (key == "fox") {
    setFoxId(value.c_str());
  } else if (key == "mode") {
    value.toLowerCase();
    if (value == "beacon" || value == "continuous") {
      config.beaconMode = true;
    } else if (value == "fox" || value == "scheduled") {
      config.beaconMode = false;
    } else {
      Serial.println(F("Use: set mode fox|beacon"));
      return;
    }
  } else if (key == "fox_sync") {
    bool parsed = false;
    if (!parseBoolValue(value, &parsed)) {
      Serial.println(F("Use: set fox_sync on|off"));
      return;
    }
    config.foxSyncEnabled = parsed;
  } else if (key == "beacon_id") {
    config.beaconIdIntervalSeconds = constrain(value.toInt(), 10, 600);
  } else if (key == "startup") {
    config.startupDelaySeconds = constrain(value.toInt(), 0, 3600);
  } else if (key == "tx") {
    config.transmitSeconds = constrain(value.toInt(), 3, 600);
  } else if (key == "idle") {
    config.idleSeconds = constrain(value.toInt(), 5, 3600);
  } else if (key == "wpm") {
    config.cwWpm = constrain(value.toInt(), 5, 35);
  } else if (key == "tone") {
    config.cwToneHz = constrain(value.toInt(), 300, 1800);
  } else if (key == "warble_low") {
    config.warbleLowHz = constrain(value.toInt(), 300, 1800);
  } else if (key == "warble_high") {
    config.warbleHighHz = constrain(value.toInt(), 300, 1800);
  } else if (key == "warble_step") {
    config.warbleStepMs = constrain(value.toInt(), 100, 3000);
  } else if (key == "lead") {
    config.pttLeadMs = constrain(value.toInt(), 50, 3000);
  } else if (key == "tail") {
    config.pttTailMs = constrain(value.toInt(), 50, 3000);
  } else if (key == "ptt") {
    value.toLowerCase();
    if (value == "active_low") {
      config.pttActiveLow = true;
    } else if (value == "active_high") {
      config.pttActiveLow = false;
    } else {
      Serial.println(F("Use: set ptt active_low|active_high"));
      return;
    }
  } else if (key == "warble") {
    bool parsed = false;
    if (!parseBoolValue(value, &parsed)) {
      Serial.println(F("Use: set warble on|off"));
      return;
    }
    config.warbleEnabled = parsed;
  } else if (key == "battery") {
    bool parsed = false;
    if (!parseBoolValue(value, &parsed)) {
      Serial.println(F("Use: set battery on|off"));
      return;
    }
    config.batteryEnabled = parsed;
  } else if (key == "battery_scale") {
    config.batteryScale = constrain(value.toFloat(), 1.0f, 10.0f);
  } else if (key == "low_battery") {
    config.lowBatteryVoltage = constrain(value.toFloat(), 2.5f, 15.0f);
  } else if (key == "wifi_ap") {
    bool parsed = false;
    if (!parseBoolValue(value, &parsed)) {
      Serial.println(F("Use: set wifi_ap on|off"));
      return;
    }
    config.wifiApEnabled = parsed;
    if (parsed && !webAdminIsRunning()) {
      webAdminInit("9M2PJU-Fox");
    } else if (!parsed && webAdminIsRunning()) {
      webAdminStop();
    }
  } else if (key == "wifi_ap_timeout") {
    config.wifiApTimeoutMinutes = constrain(value.toInt(), 0, 1440);
  } else if (key == "wifi_password") {
    value.trim();
    if (value.length() == 0 || value.equalsIgnoreCase("empty") ||
        value.equalsIgnoreCase("open") || value.equalsIgnoreCase("none")) {
      setWifiPassword("");
    } else if (value.length() < 8) {
      Serial.println(F("Password needs 8-64 chars, or empty for open."));
      return;
    } else {
      if (value.length() > 64) value = value.substring(0, 64);
      setWifiPassword(value.c_str());
    }
    if (webAdminIsRunning()) {
      webAdminStop();
      webAdminInit("9M2PJU-Fox");
    }
  } else if (key == "eco_mode") {
    bool parsed = false;
    if (!parseBoolValue(value, &parsed)) {
      Serial.println(F("Use: set eco_mode on|off"));
      return;
    }
    config.displayEcoMode = parsed;
  } else {
    Serial.println(F("Unknown setting. Type show for command list."));
    return;
  }

  markConfigDirty();
  Serial.println(F("Saved."));
  printConfig();
}

static bool parseOnOffToken(const char *s, bool *out) {
  String v = String(s);
  v.trim();
  v.toLowerCase();
  return parseBoolValue(v, out);
}

void applyPendingConfig(const PendingConfig &p) {
  // Callsign / fox: same normalize + truncate rules as serial path.
  if (p.call[0]) {
    String v = String(p.call);
    v.trim();
    if (v.length()) {
      String n = normalizeId(v);
      setCallSign(n.c_str());
    }
  }
  if (p.fox[0]) {
    String v = String(p.fox);
    v.trim();
    if (v.length()) {
      String n = normalizeId(v);
      setFoxId(n.c_str());
    }
  }
  if (p.mode[0]) {
    String m = String(p.mode);
    m.toLowerCase();
    if (m == "beacon" || m == "continuous") config.beaconMode = true;
    else if (m == "fox" || m == "scheduled") config.beaconMode = false;
  }
  if (p.foxSync[0]) { bool b; if (parseOnOffToken(p.foxSync, &b)) config.foxSyncEnabled = b; }
  if (p.ptt[0]) {
    String v = String(p.ptt);
    v.toLowerCase();
    if (v == "active_low") config.pttActiveLow = true;
    else if (v == "active_high") config.pttActiveLow = false;
  }
  if (p.warble[0]) { bool b; if (parseOnOffToken(p.warble, &b)) config.warbleEnabled = b; }
  if (p.battery[0]) { bool b; if (parseOnOffToken(p.battery, &b)) config.batteryEnabled = b; }
  if (p.wifiAp[0]) { bool b; if (parseOnOffToken(p.wifiAp, &b)) config.wifiApEnabled = b; }
  if (p.ecoMode[0]) { bool b; if (parseOnOffToken(p.ecoMode, &b)) config.displayEcoMode = b; }
  if (p.startup >= 0) config.startupDelaySeconds = constrain(p.startup, 0L, 3600L);
  if (p.tx >= 0) config.transmitSeconds = constrain(p.tx, 3L, 600L);
  if (p.idle >= 0) config.idleSeconds = constrain(p.idle, 5L, 3600L);
  if (p.beaconId >= 0) config.beaconIdIntervalSeconds = constrain((int)p.beaconId, 10, 600);
  if (p.wpm >= 0) config.cwWpm = constrain((int)p.wpm, 5, 35);
  if (p.tone >= 0) config.cwToneHz = constrain((int)p.tone, 300, 1800);
  if (p.warbleLow >= 0) config.warbleLowHz = constrain((int)p.warbleLow, 300, 1800);
  if (p.warbleHigh >= 0) config.warbleHighHz = constrain((int)p.warbleHigh, 300, 1800);
  if (p.warbleStep >= 0) config.warbleStepMs = constrain((int)p.warbleStep, 100, 3000);
  if (p.lead >= 0) config.pttLeadMs = constrain((int)p.lead, 50, 3000);
  if (p.tail >= 0) config.pttTailMs = constrain((int)p.tail, 50, 3000);
  if (p.batteryScale >= 0) config.batteryScale = constrain(p.batteryScale, 1.0f, 10.0f);
  if (p.lowBattery >= 0) config.lowBatteryVoltage = constrain(p.lowBattery, 2.5f, 15.0f);
  if (p.wifiApTimeout >= 0) config.wifiApTimeoutMinutes = constrain((int)p.wifiApTimeout, 0, 1440);
  if (p.hasWifiPassword) {
    String pw = String(p.wifiPassword);
    pw.trim();
    if (pw.length() == 0) {
      setWifiPassword("");
    } else if (pw.length() >= 8) {
      if (pw.length() > 64) pw = pw.substring(0, 64);
      setWifiPassword(pw.c_str());
    }
    // Short non-empty passwords are ignored to avoid locking the user out.
  }
  markConfigDirty();
  if (config.batteryEnabled) analogSetPinAttenuation(BATTERY_PIN, ADC_11db);
  updateWebSnapshot();
  bool wantAp = config.wifiApEnabled;
  bool haveAp = webAdminIsRunning();
  if (wantAp && !haveAp) webAdminInit("9M2PJU-Fox");
  else if (!wantAp && haveAp) webAdminStop();
  else if (wantAp && haveAp) { webAdminStop(); webAdminInit("9M2PJU-Fox"); }
}

void safeReboot() {
  Serial.println(F("Rebooting."));
  safeRadioOff();
  if (webAdminIsRunning()) webAdminStop();
  flushConfigIfDirty(true);
  delay(150);
  ESP.restart();
}

// Apply deferred on-screen menu AP/mode changes when TX is idle.
void serviceMenuPending() {
  bool doAp = false, doMode = false;
  portENTER_CRITICAL(&webMux);
  doAp = menuApPending; menuApPending = false;
  doMode = menuModePending; menuModePending = false;
  portEXIT_CRITICAL(&webMux);
  if (doAp) {
    bool wantAp = config.wifiApEnabled;
    bool haveAp = webAdminIsRunning();
    if (wantAp && !haveAp) webAdminInit("9M2PJU-Fox");
    else if (!wantAp && haveAp) webAdminStop();
    updateWebSnapshot();
  }
  if (doMode) {
    enterState(config.beaconMode ? BeaconState::ContinuousTransmit : BeaconState::Idle);
    updateWebSnapshot();
  }
}

void serviceWebRequests() {
  bool doTest = false, doPtt = false, doDefaults = false, doReboot = false, doCfg = false;
  PendingConfig cfg;
  portENTER_CRITICAL(&webMux);
  doTest = webTestRequested; webTestRequested = false;
  doPtt = webPttRequested; webPttRequested = false;
  doDefaults = webDefaultsRequested; webDefaultsRequested = false;
  doReboot = webRebootRequested; webRebootRequested = false;
  doCfg = webConfigPending; webConfigPending = false;
  if (doCfg) cfg = pendingConfig;
  portEXIT_CRITICAL(&webMux);

  if (doCfg) {
    applyPendingConfig(cfg);
    Serial.println(F("Web config saved."));
  }
  if (doDefaults) {
    loadDefaultConfig();
    markConfigDirty();
    Serial.println(F("Web defaults restored."));
  }
  if (doTest) {
    if (canQueueTestTx()) {
      forceTransmit = true;
      Serial.println(F("Web test transmission queued."));
    } else {
      Serial.println(F("Web test refused: busy."));
    }
  }
  if (doPtt) {
    if (!faultHalt && state != BeaconState::LowBatteryHalt &&
        state != BeaconState::Transmitting && state != BeaconState::ContinuousTransmit) {
      testPttOnly();
    } else {
      Serial.println(F("PTT test refused: busy, low battery, or fault halt."));
    }
  }
  if (doReboot) safeReboot();
}

void handleCommand(String command) {
  command.trim();
  if (!command.length()) {
    return;
  }

  String lower = command;
  lower.toLowerCase();

  if (lower == "show" || lower == "help" || lower == "?") {
    printConfig();
  } else if (lower == "test") {
    if (canQueueTestTx()) {
      forceTransmit = true;
      Serial.println(F("Test transmission queued."));
    } else {
      Serial.println(F("Busy: test ignored during TX."));
    }
  } else if (lower == "ptt_test") {
    if (faultHalt) {
      Serial.println(F("Fault halt active: clearfault first."));
    } else if (state == BeaconState::Transmitting || state == BeaconState::ContinuousTransmit) {
      Serial.println(F("Busy: PTT test refused during TX."));
    } else {
      testPttOnly();
    }
  } else if (lower == "sync") {
    if (state == BeaconState::Transmitting || state == BeaconState::ContinuousTransmit) {
      Serial.println(F("Busy: sync deferred until TX ends."));
    } else {
      enterState(config.beaconMode ? BeaconState::ContinuousTransmit : BeaconState::Idle);
      Serial.println(F("Cycle clock reset. Use with all foxes to re-align slots."));
    }
  } else if (lower == "defaults") {
    loadDefaultConfig();
    markConfigDirty();
    Serial.println(F("Compile-time defaults restored and saved."));
    printConfig();
  } else if (lower == "clearfault") {
    clearFaultHalt("serial");
  } else if (lower == "reboot") {
    safeReboot();
  } else if (lower.startsWith("set ")) {
    const int firstSpace = command.indexOf(' ');
    const int secondSpace = command.indexOf(' ', firstSpace + 1);
    if (secondSpace < 0) {
      Serial.println(F("Use: set <name> <value>"));
      return;
    }
    String key = command.substring(firstSpace + 1, secondSpace);
    String value = command.substring(secondSpace + 1);
    handleSetCommand(key, value);
  } else {
    Serial.println(F("Unknown command. Type show for help."));
  }
}

void readSerialCommands() {
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      handleCommand(serialLine);
      serialLine = "";
    } else if (serialLine.length() < 120) {
      serialLine += c;
    }
  }
}

// Test TX may only queue from radio-off states. Queueing mid-TX would fire a
// back-to-back slot right after the current one ends.
bool canQueueTestTx() {
  if (faultHalt) return false;
  return state == BeaconState::Idle || state == BeaconState::StartupDelay;
}

// --- OneButton handlers ---

static void onSingleClick() {
  if (millis() < BUTTON_BOOT_IGNORE_MS) return;
  lastActivityAt = millis();
  if (displaySleeping) {
    displaySleeping = false;
    displayPower(true);
    return;
  }
  if (displayMode == DisplayMode::Menu) {
    menuIndex = (menuIndex + 1) % MENU_COUNT;
  } else if (displayMode == DisplayMode::Status) {
    if (canQueueTestTx()) {
      forceTransmit = true;
      Serial.println(F("Button test transmission queued."));
    } else {
      Serial.println(F("Busy: test ignored during TX."));
    }
  }
}

static void onDoubleClick() {
  if (millis() < BUTTON_BOOT_IGNORE_MS) return;
  lastActivityAt = millis();
  if (displaySleeping) {
    displaySleeping = false;
    displayPower(true);
    return;
  }
  if (displayMode == DisplayMode::Menu) {
    menuToggle(menuIndex);
  } else if (displayMode == DisplayMode::Status) {
    displayMode = DisplayMode::Menu;
    displayModeEnteredAt = millis();
    menuIndex = 0;
  }
}

static void onLongPress() {
  if (millis() < BUTTON_BOOT_IGNORE_MS) return;
  lastActivityAt = millis();
  if (faultHalt) {
    clearFaultHalt("button");
    displayMode = DisplayMode::Status;
    displayModeEnteredAt = millis();
    return;
  }
  if (displayMode == DisplayMode::Menu) {
    displayMode = DisplayMode::Status;
    displayModeEnteredAt = millis();
  }
}

void checkButton() {
#ifdef BUTTON_PIN
  button.tick();
#endif
}

void setup() {
  // Learn PTT polarity before touching the pin so an active-low board never
  // sees a key-up glitch. Then hold the safe level across the boot window.
  preferences.begin("foxbeacon", true);
  bool pttLowEarly = preferences.getBool("pttLow", DEFAULT_PTT_ACTIVE_LOW);
  preferences.end();
  config.pttActiveLow = pttLowEarly;
  pttHoldSafe();
  digitalWrite(PTT_PIN, pttSafeGpioLevel());
  pinMode(PTT_PIN, OUTPUT);
  digitalWrite(PTT_PIN, pttSafeGpioLevel());
  pinMode(LED_PIN, OUTPUT);
#ifdef BUTTON_PIN
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  button.attachClick(onSingleClick);
  button.attachDoubleClick(onDoubleClick);
  button.attachLongPressStart(onLongPress);
  button.setPressMs(2000);  // long press = 2 seconds
#endif
  setPtt(false);
  setLed(false);

  Serial.begin(SERIAL_BAUD);
  delay(250);
  recordBootFault();

  loadConfig();
  displayInit(config.callSign, FIRMWARE_VERSION);
  displayMode = DisplayMode::StartupScreen;
  displayModeEnteredAt = millis();
  lastActivityAt = millis();
  setPtt(false);
  pttHoldSafe();

  audioInit();
  audioOff();

  if (config.batteryEnabled) {
    analogSetPinAttenuation(BATTERY_PIN, ADC_11db);
    cachedBatteryVoltage = sampleBatteryRaw();
    lastBatterySampleAt = millis();
  }

  if (config.wifiApEnabled) {
    webAdminInit("9M2PJU-Fox");
  }
  updateWebSnapshot();

#ifdef FOX_USE_NEW_LEDC
  {
    esp_task_wdt_config_t wdtCfg = {
      .timeout_ms = 30000,
      .idle_core_mask = 0,
      .trigger_panic = true,
    };
    esp_task_wdt_init(&wdtCfg);
  }
#else
  esp_task_wdt_init(30, true);
#endif
  esp_task_wdt_add(NULL);

  printConfig();
  Serial.println(F("Beacon armed."));
  const uint32_t startupDelay = resolvedStartupDelaySeconds();
  enterState(startupDelay > 0 ? BeaconState::StartupDelay
                               : (config.beaconMode ? BeaconState::ContinuousTransmit : BeaconState::Idle));
}

void loop() {
  petWatchdog();
  readSerialCommands();
  checkButton();
  webAdminLoop();
  serviceWebRequests();
  flushConfigIfDirty(false);
  // Deferred menu AP/mode flips only when TX is not active.
  if (state != BeaconState::Transmitting && state != BeaconState::ContinuousTransmit) {
    serviceMenuPending();
  }

  if (faultHalt) {
    safeRadioOff();
    forceTransmit = false;
    blinkLed(LED_LOW_BATTERY_BLINK_MS);
    static uint32_t lastFaultSnap = 0;
    if (millis() - lastFaultSnap >= 5000UL) {
      updateWebSnapshot();
      lastFaultSnap = millis();
    }
    updateDisplay();
    delay(5);
    return;
  }

  if (state != BeaconState::LowBatteryHalt && isLowBattery()) {
    Serial.printf("Low battery halt: %.2f V\n", readBatteryVoltage());
    forceTransmit = false;
    safeRadioOff();
    enterState(BeaconState::LowBatteryHalt);
    updateWebSnapshot();
  } else if (state == BeaconState::LowBatteryHalt && isBatteryRecovered()) {
    Serial.printf("Battery recovered: %.2f V, resuming.\n", readBatteryVoltageFresh());
    lowBatteryTripStreak = 0;
    enterState(config.beaconMode ? BeaconState::ContinuousTransmit : BeaconState::Idle);
    updateWebSnapshot();
  }

  if (forceTransmit && canQueueTestTx()) {
    forceTransmit = false;
    transmitBeacon();
    if (state != BeaconState::LowBatteryHalt) {
      enterState(config.beaconMode ? BeaconState::ContinuousTransmit : BeaconState::Idle);
    }
  }

  switch (state) {
    case BeaconState::StartupDelay: {
      blinkLed(LED_IDLE_BLINK_MS);
      const uint32_t startupDelay = resolvedStartupDelaySeconds();
      if (elapsedSince(stateStartedAt) >= startupDelay * 1000UL) {
        enterState(config.beaconMode ? BeaconState::ContinuousTransmit : BeaconState::Idle);
      }
      break;
    }

    case BeaconState::Idle:
      blinkLed(LED_IDLE_BLINK_MS);
      if (elapsedSince(stateStartedAt) >= config.idleSeconds * 1000UL) {
        enterState(BeaconState::Transmitting);
      }
      break;

    case BeaconState::Transmitting:
      blinkLed(LED_TX_BLINK_MS);
      transmitBeacon();
      enterState(BeaconState::Idle);
      break;

    case BeaconState::ContinuousTransmit:
      runContinuousBeacon();
      if (isLowBattery()) {
        enterState(BeaconState::LowBatteryHalt);
      }
      break;

    case BeaconState::LowBatteryHalt:
      blinkLed(LED_LOW_BATTERY_BLINK_MS);
      break;
  }

  static uint32_t lastSnapRefreshMs = 0;
  if (webAdminIsRunning() && millis() - lastSnapRefreshMs >= 5000UL) {
    updateWebSnapshot();
    lastSnapRefreshMs = millis();
  }

  updateDisplay();
  // Lower CPU/battery use when sitting idle with radio off, but keep the
  // button responsive: poll every 5ms instead of one 50ms block.
  if ((state == BeaconState::Idle || state == BeaconState::StartupDelay ||
       state == BeaconState::LowBatteryHalt) &&
      !webAdminIsRunning()) {
    for (uint8_t i = 0; i < 10; i++) {
      checkButton();
      petWatchdog();
      delay(5);
    }
  } else {
    delay(5);
  }
}
