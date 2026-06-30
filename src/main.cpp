#include <Arduino.h>
#include <Preferences.h>
#include "beacon_config.h"

constexpr uint8_t AUDIO_CHANNEL = 0;
constexpr uint8_t AUDIO_RESOLUTION_BITS = 10;
constexpr uint16_t AUDIO_DUTY = 512;
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t BUTTON_HOLD_MS = 35;
constexpr uint32_t LED_IDLE_BLINK_MS = 1800;
constexpr uint32_t LED_TX_BLINK_MS = 160;
constexpr uint32_t LED_LOW_BATTERY_BLINK_MS = 350;
constexpr uint32_t PTT_TEST_MS = 1200;

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
};

enum class BeaconState {
  StartupDelay,
  Idle,
  Transmitting,
  LowBatteryHalt,
};

Preferences preferences;
BeaconConfig config;
BeaconState state = BeaconState::StartupDelay;
uint32_t stateStartedAt = 0;
uint32_t lastLedToggleAt = 0;
bool ledOn = false;
bool forceTransmit = false;
String serialLine;

String normalizeId(String value) {
  value.trim();
  value.toUpperCase();
  String result;
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value.charAt(i);
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '/' || c == '-') {
      result += c;
    }
  }
  return result.length() ? result : "FOX";
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

void audioOff() {
  ledcWriteTone(AUDIO_CHANNEL, 0);
  ledcWrite(AUDIO_CHANNEL, 0);
}

void audioTone(uint16_t frequencyHz) {
  ledcWriteTone(AUDIO_CHANNEL, frequencyHz);
  ledcWrite(AUDIO_CHANNEL, AUDIO_DUTY);
}

void setPtt(bool active) {
  const bool outputHigh = config.pttActiveLow ? !active : active;
  digitalWrite(PTT_PIN, outputHigh ? HIGH : LOW);
}

float readBatteryVoltage() {
  if (!config.batteryEnabled) {
    return 99.0f;
  }

  constexpr uint8_t samples = 12;
  uint32_t totalMillivolts = 0;
  for (uint8_t i = 0; i < samples; ++i) {
    totalMillivolts += analogReadMilliVolts(BATTERY_PIN);
    delay(3);
  }

  const float pinVoltage = (totalMillivolts / static_cast<float>(samples)) / 1000.0f;
  return pinVoltage * config.batteryScale;
}

bool isLowBattery() {
  return config.batteryEnabled && readBatteryVoltage() <= config.lowBatteryVoltage;
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

void keyedDelay(uint32_t durationMs) {
  const uint32_t startedAt = millis();
  while (elapsedSince(startedAt) < durationMs) {
    delay(1);
  }
}

void sendMorseText(const String &text) {
  const uint32_t dotMs = 1200UL / max<uint16_t>(config.cwWpm, 5);
  const String id = normalizeId(text);

  for (size_t charIndex = 0; charIndex < id.length(); ++charIndex) {
    const char c = id.charAt(charIndex);
    if (c == ' ') {
      keyedDelay(dotMs * 7);
      continue;
    }

    const char *pattern = morseFor(c);
    for (size_t i = 0; pattern[i] != '\0'; ++i) {
      audioTone(config.cwToneHz);
      keyedDelay(pattern[i] == '-' ? dotMs * 3 : dotMs);
      audioOff();
      keyedDelay(dotMs);
    }

    keyedDelay(dotMs * 2);
  }
}

void sendWarbleUntil(uint32_t untilMs) {
  bool high = false;
  while (static_cast<int32_t>(untilMs - millis()) > 0) {
    audioTone(high ? config.warbleHighHz : config.warbleLowHz);
    high = !high;
    const uint32_t step = min<uint32_t>(config.warbleStepMs, max<int32_t>(untilMs - millis(), 0));
    keyedDelay(step);
  }
  audioOff();
}

void saveConfig() {
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
  preferences.end();
}

void loadDefaultConfig() {
  config = BeaconConfig();
  config.callSign = normalizeId(config.callSign);
  config.foxId = normalizeId(config.foxId);
}

void loadConfig() {
  loadDefaultConfig();

  preferences.begin("foxbeacon", true);
  config.callSign = normalizeId(preferences.getString("call", config.callSign));
  config.foxId = normalizeId(preferences.getString("fox", config.foxId));
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
}

void printConfig() {
  Serial.println();
  Serial.println(F("ESP32 Fox Hunt Beacon"));
  Serial.println(F("---------------------"));
  Serial.printf("Callsign:       %s\n", config.callSign.c_str());
  Serial.printf("Fox ID:         %s\n", config.foxId.c_str());
  Serial.printf("Startup delay:  %lu s\n", static_cast<unsigned long>(config.startupDelaySeconds));
  Serial.printf("Transmit time:  %lu s\n", static_cast<unsigned long>(config.transmitSeconds));
  Serial.printf("Idle time:      %lu s\n", static_cast<unsigned long>(config.idleSeconds));
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
    Serial.printf(", now %.2f V", readBatteryVoltage());
  }
  Serial.println();
  Serial.println();
  Serial.println(F("Commands:"));
  Serial.println(F("  show"));
  Serial.println(F("  test"));
  Serial.println(F("  ptt_test"));
  Serial.println(F("  defaults"));
  Serial.println(F("  reboot"));
  Serial.println(F("  set call 9M2PJU"));
  Serial.println(F("  set fox FOX1"));
  Serial.println(F("  set startup 300"));
  Serial.println(F("  set tx 30"));
  Serial.println(F("  set idle 90"));
  Serial.println(F("  set wpm 12"));
  Serial.println(F("  set tone 700"));
  Serial.println(F("  set warble on|off"));
  Serial.println(F("  set ptt active_low|active_high"));
  Serial.println(F("  set battery on|off"));
  Serial.println(F("  set battery_scale 2.0"));
  Serial.println(F("  set low_battery 3.4"));
  Serial.println();
}

void enterState(BeaconState nextState) {
  state = nextState;
  stateStartedAt = millis();
  lastLedToggleAt = millis();
}

void transmitBeacon() {
  Serial.println(F("TX start"));
  setLed(true);
  setPtt(true);
  keyedDelay(config.pttLeadMs);

  const uint32_t transmissionEndsAt = millis() + (config.transmitSeconds * 1000UL);
  sendMorseText(config.callSign);
  keyedDelay(500);
  sendMorseText(config.foxId);
  keyedDelay(500);

  if (config.warbleEnabled && static_cast<int32_t>(transmissionEndsAt - millis()) > 0) {
    sendWarbleUntil(transmissionEndsAt);
  } else {
    audioOff();
  }

  keyedDelay(config.pttTailMs);
  audioOff();
  setPtt(false);
  setLed(false);
  Serial.println(F("TX end"));
}

void testPttOnly() {
  Serial.println(F("PTT test start"));
  audioOff();
  setPtt(true);
  setLed(true);
  keyedDelay(PTT_TEST_MS);
  setPtt(false);
  setLed(false);
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
    config.callSign = normalizeId(value);
  } else if (key == "fox") {
    config.foxId = normalizeId(value);
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
  } else {
    Serial.println(F("Unknown setting. Type show for command list."));
    return;
  }

  saveConfig();
  Serial.println(F("Saved."));
  printConfig();
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
    forceTransmit = true;
    Serial.println(F("Test transmission queued."));
  } else if (lower == "ptt_test") {
    testPttOnly();
  } else if (lower == "defaults") {
    loadDefaultConfig();
    saveConfig();
    Serial.println(F("Compile-time defaults restored and saved."));
    printConfig();
  } else if (lower == "reboot") {
    Serial.println(F("Rebooting."));
    delay(100);
    ESP.restart();
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

void checkButton() {
  static bool wasPressed = false;
  static uint32_t pressedAt = 0;
  const bool pressed = digitalRead(BUTTON_PIN) == LOW;

  if (pressed && !wasPressed) {
    pressedAt = millis();
  }

  if (!pressed && wasPressed && elapsedSince(pressedAt) >= BUTTON_HOLD_MS) {
    forceTransmit = true;
    Serial.println(F("Button test transmission queued."));
  }

  wasPressed = pressed;
}

void setup() {
  pinMode(PTT_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  setPtt(false);
  setLed(false);

  Serial.begin(SERIAL_BAUD);
  delay(250);

  loadConfig();
  setPtt(false);

  ledcSetup(AUDIO_CHANNEL, config.cwToneHz, AUDIO_RESOLUTION_BITS);
  ledcAttachPin(AUDIO_PIN, AUDIO_CHANNEL);
  audioOff();

  if (config.batteryEnabled) {
    analogSetPinAttenuation(BATTERY_PIN, ADC_11db);
  }

  printConfig();
  Serial.println(F("Beacon armed."));
  enterState(config.startupDelaySeconds > 0 ? BeaconState::StartupDelay : BeaconState::Idle);
}

void loop() {
  readSerialCommands();
  checkButton();

  if (state != BeaconState::LowBatteryHalt && isLowBattery()) {
    Serial.printf("Low battery halt: %.2f V\n", readBatteryVoltage());
    audioOff();
    setPtt(false);
    enterState(BeaconState::LowBatteryHalt);
  }

  if (forceTransmit && state != BeaconState::LowBatteryHalt) {
    forceTransmit = false;
    transmitBeacon();
    enterState(BeaconState::Idle);
  }

  switch (state) {
    case BeaconState::StartupDelay:
      blinkLed(LED_IDLE_BLINK_MS);
      if (elapsedSince(stateStartedAt) >= config.startupDelaySeconds * 1000UL) {
        enterState(BeaconState::Idle);
      }
      break;

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

    case BeaconState::LowBatteryHalt:
      blinkLed(LED_LOW_BATTERY_BLINK_MS);
      break;
  }

  delay(5);
}
