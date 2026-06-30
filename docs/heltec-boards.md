# Heltec Board Support

This firmware can build for several popular Heltec ESP32 boards. Heltec boards
with LoRa radios are supported as ESP32 controller boards for this project: the
firmware still keys an external FM radio or transmitter module through PTT and
audio wiring. It does not use the onboard SX127x/SX126x LoRa radio as an ARDF FM
fox transmitter.

## Supported PlatformIO Environments

Use these names with PlatformIO:

| Environment | PlatformIO board ID | Notes |
| --- | --- | --- |
| `heltec-wifi-kit-32` | `heltec_wifi_kit_32` | Original Heltec WiFi Kit 32. |
| `heltec-wifi-kit-32-v2` | `heltec_wifi_kit_32_v2` | WiFi Kit 32 V2. |
| `heltec-wifi-kit-32-v3` | `heltec_wifi_kit_32_V3` | ESP32-S3 WiFi Kit 32 V3. |
| `heltec-wifi-lora-32` | `heltec_wifi_lora_32` | Original WiFi LoRa 32. Onboard LoRa is not used. |
| `heltec-wifi-lora-32-v2` | `heltec_wifi_lora_32_V2` | WiFi LoRa 32 V2. Onboard LoRa is not used. |
| `heltec-wifi-lora-32-v3` | `heltec_wifi_lora_32_V3` | ESP32-S3 WiFi LoRa 32 V3. Onboard LoRa is not used. |
| `heltec-wireless-stick` | `heltec_wireless_stick` | Wireless Stick. Onboard LoRa is not used. |
| `heltec-wireless-stick-lite` | `heltec_wireless_stick_lite` | Wireless Stick Lite. Onboard LoRa is not used. |
| `heltec-wireless-stick-lite-v3` | `esp32-s3-devkitc-1` plus Heltec Arduino variant | Wireless Stick Lite V3 compatibility build. Onboard LoRa is not used. |
| `heltec-wireless-tracker` | `esp32-s3-devkitc-1` compatibility build | Wireless Tracker uses ESP32-S3, SX1262 LoRa, and GNSS. PlatformIO does not provide a dedicated board ID in the installed espressif32 package, so this is a compatibility build that needs header-pin verification. |

Example build:

```sh
pio run -e heltec-wifi-lora-32-v3
```

Example upload:

```sh
pio run -e heltec-wifi-lora-32-v3 -t upload
```

## Starter Pin Mapping

The Heltec environments override the default pins so the firmware does not use
the generic ESP32 DevKit defaults blindly.

Classic ESP32 Heltec boards:

| Function | GPIO |
| --- | ---: |
| PTT output | 13 |
| Audio output | 17 |
| Status LED | 25 |
| Test button | 0 |
| Battery ADC | 36 |

ESP32-S3 Heltec boards and compatibility builds:

| Function | GPIO |
| --- | ---: |
| PTT output | 6 |
| Audio output | 7 |
| Status LED | 35 |
| Test button | 0 |
| Battery ADC | 1 |

These are starter mappings for firmware builds and bench testing. Before wiring
a radio, compare the pins with the exact board revision, silkscreen, schematic,
and pin map. Heltec boards often reserve pins for OLED, LoRa, GNSS, battery, or
power-control circuits.

## Compatibility Build Notes

Heltec Wireless Tracker is based on ESP32-S3 and includes SX1262 LoRa plus GNSS.
This project does not currently use those onboard radio/GNSS features. The
`heltec-wireless-tracker` environment exists so users can build the controller
firmware and wire external PTT/audio lines, but it should be treated as
experimental until tested on real hardware.

`heltec-wireless-stick-lite-v3` is also a compatibility build because this
PlatformIO package includes the Arduino variant but not a dedicated board JSON.
It compiles, but should be pin-checked on real hardware before connecting a
radio.

For a normal VHF/UHF FM fox hunt beacon, connect the Heltec board to an external
handheld radio or FM transmitter module. Do not assume the onboard LoRa radio is
a replacement for a legal FM ARDF transmitter.

## VS Code Usage

In VS Code with PlatformIO:

1. Open this repository folder.
2. Open `platformio.ini`.
3. Change `default_envs` to the Heltec environment you want, or choose the
   environment from `Project Tasks`.
4. Build.
5. Upload.
6. Open Serial Monitor and run `show` and `ptt_test`.

Keep the radio disconnected for the first upload and first `ptt_test`.
