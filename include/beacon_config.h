#pragma once

// Main user configuration file.
//
// Edit these values before building if you want fixed defaults in the firmware.
// After flashing, Serial Monitor commands can override these values and save
// them to ESP32 flash. Use the "defaults" serial command to restore these
// compile-time defaults.

// What the beacon sends at the start of every transmission.
#define DEFAULT_CALLSIGN "9M2PJU"
#define DEFAULT_FOX_ID "FOX1"

// Main timing.
#define DEFAULT_STARTUP_DELAY_SECONDS 10
#define DEFAULT_TRANSMIT_SECONDS 30
#define DEFAULT_IDLE_SECONDS 90

// Morse/CW ID.
#define DEFAULT_CW_WPM 12
#define DEFAULT_CW_TONE_HZ 700

// Optional attention tone after the CW ID. Set DEFAULT_WARBLE_ENABLED to 0 if
// you only want the callsign and fox ID.
#define DEFAULT_WARBLE_ENABLED 1
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
