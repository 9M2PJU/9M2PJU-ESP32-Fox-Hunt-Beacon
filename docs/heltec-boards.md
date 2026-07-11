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
| `heltec-wireless-tracker` | `esp32-s3-devkitc-1` compatibility build | Wireless Tracker (ESP32-S3, SX1262 LoRa, GNSS, 0.96" TFT ST7735 160x80). Onboard LoRa/GNSS not used. |
| `heltec-vision-master-t190` | `esp32-s3-devkitc-1` compatibility build | Vision Master T190 (ESP32-S3, 1.9" TFT ST7789 170x320). Onboard LoRa not used. |
| `heltec-wireless-paper` | `esp32-s3-devkitc-1` compatibility build | Wireless Paper (ESP32-S3, 2.13" E-Ink 250x122). Onboard LoRa not used. |
| `heltec-vision-master-e213` | `esp32-s3-devkitc-1` compatibility build | Vision Master E213 (ESP32-S3, 2.13" E-Ink 250x122). Onboard LoRa not used. |
| `heltec-vision-master-e290` | `esp32-s3-devkitc-1` compatibility build | Vision Master E290 (ESP32-S3, 2.9" E-Ink 296x128). Onboard LoRa not used. |
| `heltec-capsule-sensor-v3` | `esp32-s3-devkitc-1` compatibility build | Capsule Sensor V3 (ESP32-S3, no display). Onboard LoRa not used. |

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

Wireless Tracker (ESP32-S3) uses different pins due to TFT + GPS:

| Function | GPIO |
| --- | ---: |
| PTT output | 7 |
| Audio output | 6 |
| Status LED | 18 |
| Test button | 0 |
| Battery ADC | 1 |

Vision Master series and Capsule Sensor V3 (ESP32-S3):

| Function | GPIO |
| --- | ---: |
| PTT output | 4 |
| Audio output | 3 |
| Status LED | 18 |
| Test button | 0 |
| Battery ADC | 6 |

These are starter mappings for firmware builds and bench testing. Before wiring
a radio, compare the pins with the exact board revision, silkscreen, schematic,
and pin map. Heltec boards often reserve pins for OLED, LoRa, GNSS, battery, or
power-control circuits.

## Compatibility Build Notes

Several newer Heltec boards (Wireless Tracker, Vision Master series, Wireless
Paper, Capsule Sensor V3) do not have dedicated PlatformIO board IDs in the
installed espressif32 package. These environments use the generic
`esp32-s3-devkitc-1` board definition with explicit pin mappings and display
configuration via build flags. They compile successfully, but should be
pin-checked on real hardware before connecting a radio.

The Wireless Tracker has a 0.96" TFT ST7735 display (160x80). The Vision Master
T190 has a 1.9" TFT ST7789 display (170x320). The Wireless Paper and Vision
Master E213 have 2.13" E-Ink displays (250x122). The Vision Master E290 has a
2.9" E-Ink display (296x128). E-Ink displays use the GxEPD2 library and support
the on-screen menu and status screen, but refresh more slowly (1-4 seconds) than
OLED or TFT displays.

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
