#pragma once

#include <Arduino.h>

// Firmware version shown on startup screen and web admin UI.
#define FIRMWARE_VERSION "1.2.0"

// Preferences schema version. Bumped when new keys are added.
#define CONFIG_VERSION 2

// Main user configuration file.
//
// Edit these values before building if you want fixed defaults in the firmware.
// After flashing, Serial Monitor commands can override these values and save
// them to ESP32 flash. Use the "defaults" serial command to restore these
// compile-time defaults.

// What the beacon sends at the start of every transmission.
#define DEFAULT_CALLSIGN "9M2PJU"
#define DEFAULT_FOX_ID "MOE"

// Main timing. Defaults follow the IARU Region 1 ARDF 5-fox cycle: each fox
// transmits for 60 seconds, then waits 240 seconds while the other four foxes
// take their turns. The full cycle is 300 seconds (5 minutes).
#define DEFAULT_STARTUP_DELAY_SECONDS 10
#define DEFAULT_TRANSMIT_SECONDS 60
#define DEFAULT_IDLE_SECONDS 240

// Fox-slot synchronization. When enabled, the firmware derives the fox number
// from the fox ID (MOE=1, MOI=2, MOS=3, MOH=4, MO5=5) and overrides the startup
// delay with (foxNumber - 1) * transmitSeconds so that five beacons started at
// roughly the same time fall into the standard round-robin order.
#define DEFAULT_FOX_SYNC_ENABLED 1

// Continuous beacon mode (finish-line / MO6 beacon). When enabled, the beacon
// keys PTT continuously and re-sends the CW ID every beaconIdIntervalSeconds.
// This is not part of the 5-fox round-robin; it runs on a separate frequency.
#define DEFAULT_BEACON_MODE 0
#define DEFAULT_BEACON_ID_INTERVAL_SECONDS 60

// Morse/CW ID.
#define DEFAULT_CW_WPM 12
#define DEFAULT_CW_TONE_HZ 700

// Optional attention tone after the CW ID. Disabled by default for IARU
// standard compliance: competition foxes send a CW ID followed by a steady
// carrier, not a warble. Enable for training or attention-tone use.
#define DEFAULT_WARBLE_ENABLED 0
#define DEFAULT_WARBLE_LOW_HZ 700
#define DEFAULT_WARBLE_HIGH_HZ 900
#define DEFAULT_WARBLE_STEP_MS 350

// Radio keying. This is the ESP32 GPIO active level, not the radio PTT line
// polarity. For the recommended NPN/MOSFET/opto interface, a HIGH GPIO usually
// turns the interface on and pulls the radio PTT line to ground.
#define DEFAULT_PTT_ACTIVE_LOW 0
#define DEFAULT_PTT_LEAD_MS 350
#define DEFAULT_PTT_TAIL_MS 350

// Battery monitor. Leave disabled until the resistor divider is built and
// calibrated.
#define DEFAULT_BATTERY_ENABLED 0
#define DEFAULT_BATTERY_SCALE 2.0f
#define DEFAULT_LOW_BATTERY_VOLTAGE 3.40f

// Display eco mode. When enabled, the display sleeps after a short period of
// inactivity to save power — desirable for a battery-powered fox hunt beacon.
// Wakes on any button press.
#define DEFAULT_DISPLAY_ECO_MODE 1

// WiFi AP password. Empty = open network for easy field access. Set a password
// to prevent strangers from reconfiguring the beacon.
#define DEFAULT_WIFI_AP_PASSWORD ""

// Low-battery hysteresis (volts). Halt at cutoff, resume at cutoff + hysteresis
// to avoid relay-style flapping on a noisy ADC.
#define LOW_BATTERY_HYSTERESIS_V 0.20f
// Consecutive low readings required before halting TX.
#define LOW_BATTERY_TRIP_COUNT 3
// Battery sampling: cache voltage, re-sample at most this often.
#define BATTERY_SAMPLE_MS 5000UL

// E-Ink displays wear and ghost on every refresh: refresh only on content
// change, plus a slow heartbeat to keep the panel alive.
#define EINK_MIN_UPDATE_MS 5000UL
#define EINK_HEARTBEAT_MS 600000UL

// Ignore key-up voltage sag this long after PTT on (TX load dip).
#define TX_BATTERY_GRACE_MS 5000UL

// Ignore button presses this long after boot (BOOT button is strapping pin 0).
#define BUTTON_BOOT_IGNORE_MS 2000UL

// Default pins for classic ESP32 DevKit-style boards. Some board environments
// override these in platformio.ini.
#ifndef PTT_PIN
#define PTT_PIN 25
#endif

#ifndef AUDIO_PIN
#define AUDIO_PIN 26
#endif

#ifndef LED_PIN
#define LED_PIN 2
#endif

#ifndef BUTTON_PIN
#define BUTTON_PIN 0
#endif

#ifndef BATTERY_PIN
#define BATTERY_PIN 34
#endif

// Runtime configuration structure. Stored in ESP32 flash via Preferences.
// Call/fox/password are fixed buffers so the live config never grows the
// heap. Use the helpers in main.cpp to read or write them.
struct BeaconConfig {
  char callSign[13] = DEFAULT_CALLSIGN;
  char foxId[9] = DEFAULT_FOX_ID;
  uint32_t startupDelaySeconds = DEFAULT_STARTUP_DELAY_SECONDS;
  uint32_t transmitSeconds = DEFAULT_TRANSMIT_SECONDS;
  uint32_t idleSeconds = DEFAULT_IDLE_SECONDS;
  uint16_t cwWpm = DEFAULT_CW_WPM;
  uint16_t cwToneHz = DEFAULT_CW_TONE_HZ;
  uint16_t warbleLowHz = DEFAULT_WARBLE_LOW_HZ;
  uint16_t warbleHighHz = DEFAULT_WARBLE_HIGH_HZ;
  uint16_t warbleStepMs = DEFAULT_WARBLE_STEP_MS;
  uint16_t pttLeadMs = DEFAULT_PTT_LEAD_MS;
  uint16_t pttTailMs = DEFAULT_PTT_TAIL_MS;
  bool pttActiveLow = DEFAULT_PTT_ACTIVE_LOW;
  bool warbleEnabled = DEFAULT_WARBLE_ENABLED;
  bool batteryEnabled = DEFAULT_BATTERY_ENABLED;
  float batteryScale = DEFAULT_BATTERY_SCALE;
  float lowBatteryVoltage = DEFAULT_LOW_BATTERY_VOLTAGE;
  bool foxSyncEnabled = DEFAULT_FOX_SYNC_ENABLED;
  bool beaconMode = DEFAULT_BEACON_MODE;
  uint16_t beaconIdIntervalSeconds = DEFAULT_BEACON_ID_INTERVAL_SECONDS;
  bool wifiApEnabled = true;       // WiFi AP + web admin UI on/off
  uint16_t wifiApTimeoutMinutes = 10; // Auto-off AP after N minutes of no activity (0 = never)
  bool displayEcoMode = DEFAULT_DISPLAY_ECO_MODE;     // Turn off display after inactivity
  char wifiApPassword[65] = DEFAULT_WIFI_AP_PASSWORD; // Empty = open AP
};

// Delayed flash save: coalesce rapid UI/menu edits into one write.
constexpr uint32_t CONFIG_SAVE_DELAY_MS = 2000;

// Boot fault guard: halt TX after this many consecutive panic/WDT/brownout
// boots until the user clears it with the button or serial.
constexpr uint8_t BOOT_FAULT_TRIP_COUNT = 3;

// Fixed-size config update queued from the async web task to the main loop.
// Plain POD with no String heap so it can cross FreeRTOS tasks safely.
struct PendingConfig {
  char call[16] = {0};
  char fox[16] = {0};
  char mode[12] = {0};
  char foxSync[8] = {0};
  char ptt[16] = {0};
  char warble[8] = {0};
  char battery[8] = {0};
  char wifiAp[8] = {0};
  char ecoMode[8] = {0};
  char wifiPassword[65] = {0};
  bool hasWifiPassword = false;
  long startup = -1;
  long tx = -1;
  long idle = -1;
  long beaconId = -1;
  long wpm = -1;
  long tone = -1;
  long warbleLow = -1;
  long warbleHigh = -1;
  long warbleStep = -1;
  long lead = -1;
  long tail = -1;
  float batteryScale = -1.0f;
  float lowBattery = -1.0f;
  long wifiApTimeout = -1;
};

// Snapshot copied by the main loop for the async web task to read.
// No Strings, so the web task never touches heap shared with the loop.
struct WebSnapshot {
  char call[16] = {0};
  char fox[16] = {0};
  bool beaconMode = false;
  bool foxSync = true;
  uint8_t foxNum = 0;
  uint32_t startup = 0;
  uint32_t startupResolved = 0;
  uint32_t tx = 60;
  uint32_t idle = 240;
  uint16_t beaconId = 60;
  uint16_t wpm = 12;
  uint16_t tone = 700;
  bool warble = false;
  uint16_t warbleLow = 700;
  uint16_t warbleHigh = 900;
  uint16_t warbleStep = 350;
  bool pttActiveLow = false;
  uint16_t lead = 350;
  uint16_t tail = 350;
  bool batteryEnabled = false;
  float batteryScale = 2.0f;
  float lowBattery = 3.4f;
  bool wifiAp = true;
  uint16_t wifiApTimeout = 10;
  bool ecoMode = true;
  bool wifiPwSet = false;
  bool overlap = false;
  int state = 1;
  char batteryStr[16] = "N/A";
  char batteryPctStr[8] = {0};
  bool hasBatteryPct = false;
  char apSsid[40] = {0};
  char ip[20] = {0};
};

// Beacon state machine states.
enum class BeaconState {
  StartupDelay,
  Idle,
  Transmitting,
  ContinuousTransmit,
  LowBatteryHalt,
};
