#pragma once

#include <Arduino.h>

// Firmware version shown on startup screen and web admin UI.
#define FIRMWARE_VERSION "1.1.0"

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
// Defaults come from the #defines above.
struct BeaconConfig {
  String callSign = DEFAULT_CALLSIGN;
  String foxId = DEFAULT_FOX_ID;
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
  bool displayEcoMode = false;     // Turn off display after inactivity
};

// Beacon state machine states.
enum class BeaconState {
  StartupDelay,
  Idle,
  Transmitting,
  ContinuousTransmit,
  LowBatteryHalt,
};
