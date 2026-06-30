# LilyGO Board Support

This firmware can build for several popular LilyGO and TTGO ESP32 boards.
LilyGO boards with LoRa, GNSS, displays, touch controllers, or battery charger
hardware are supported as ESP32 controller boards for this project: the firmware
still keys an external FM radio or transmitter module through PTT and audio
wiring. It does not use the onboard LoRa radio as an ARDF FM fox transmitter,
and it does not use onboard displays or GNSS in the current firmware.

## Supported PlatformIO Environments

Use these names with PlatformIO:

| Environment | PlatformIO board ID | Notes |
| --- | --- | --- |
| `lilygo-t-display` | `lilygo-t-display` | Original ESP32 T-Display. Display is not used. |
| `lilygo-t-display-s3` | `lilygo-t-display-s3` | ESP32-S3 T-Display. Display is not used. |
| `lilygo-t3-s3` | `lilygo-t3-s3` | ESP32-S3 LoRa board. Onboard LoRa is not used. |
| `ttgo-lora32-v1` | `ttgo-lora32-v1` | TTGO LoRa32 OLED V1. Onboard LoRa and OLED are not used. |
| `ttgo-lora32-v2` | `ttgo-lora32-v2` | TTGO LoRa32 OLED V2. Onboard LoRa and OLED are not used. |
| `ttgo-lora32-v21` | `ttgo-lora32-v21` | TTGO LoRa32 OLED v2.1.6. Onboard LoRa and OLED are not used. |
| `ttgo-t-beam` | `ttgo-t-beam` | TTGO T-Beam with LoRa/GNSS hardware. Onboard LoRa and GNSS are not used. |
| `ttgo-t-oi-plus` | `ttgo-t-oi-plus` | ESP32-C3 T-OI Plus. Fewer GPIO pins than classic ESP32 boards. |
| `ttgo-t-watch` | `ttgo-t-watch` | T-Watch build target. Watch peripherals are not used. |
| `ttgo-t1` | `ttgo-t1` | TTGO T1 ESP32 board. |
| `ttgo-t7-v13-mini32` | `ttgo-t7-v13-mini32` | TTGO T7 V1.3 Mini32. |
| `ttgo-t7-v14-mini32` | `ttgo-t7-v14-mini32` | TTGO T7 V1.4 Mini32. |

Example build:

```sh
pio run -e ttgo-t-beam
```

Example upload:

```sh
pio run -e ttgo-t-beam -t upload
```

## Starter Pin Mapping

The LilyGO environments override the default pins so the firmware does not use
generic ESP32 DevKit defaults blindly.

Classic ESP32 LilyGO boards:

| Function | GPIO |
| --- | ---: |
| PTT output | 25 |
| Audio output | 26 |
| Status LED | 2 |
| Test button | 0 |
| Battery ADC | 34 |

Classic ESP32 LilyGO/TTGO LoRa boards:

| Function | GPIO |
| --- | ---: |
| PTT output | 25 |
| Audio output | 13 |
| Status LED | 2 |
| Test button | 0 |
| Battery ADC | 35 |

ESP32-S3 LilyGO boards:

| Function | GPIO |
| --- | ---: |
| PTT output | 6 |
| Audio output | 7 |
| Status LED | 38 |
| Test button | 0 |
| Battery ADC | 4 |

ESP32-C3 LilyGO boards:

| Function | GPIO |
| --- | ---: |
| PTT output | 7 |
| Audio output | 5 |
| Status LED | 8 |
| Test button | 9 |
| Battery ADC | 3 |

TTGO T-Beam uses the LoRa-board mapping, with status LED changed to GPIO 4.

These are starter mappings for firmware builds and bench testing. Before wiring
a radio, compare the pins with the exact board revision, silkscreen, schematic,
and pin map. LilyGO board revisions vary, and many reserve pins for TFT/OLED
displays, LoRa modules, GNSS modules, battery measurement, charging circuits, or
power-control circuits.

## LoRa, Display, And GNSS Notes

For a normal VHF/UHF FM fox hunt beacon, connect the LilyGO board to an
external handheld radio or FM transmitter module. Do not assume the onboard LoRa
radio is a replacement for a legal FM ARDF transmitter.

Boards such as TTGO LoRa32, LilyGO T3-S3, and TTGO T-Beam include LoRa radios.
This firmware does not currently transmit ARDF IDs through those radios. Boards
such as TTGO T-Beam may also include GNSS hardware, but the current firmware
does not read position or time from GNSS.

Boards such as T-Display and T-Watch include displays. The current firmware uses
Serial Monitor and LED status instead of the display.

## VS Code Usage

In VS Code with PlatformIO:

1. Open this repository folder.
2. Open `platformio.ini`.
3. Change `default_envs` to the LilyGO or TTGO environment you want, or choose
   the environment from `Project Tasks`.
4. Build.
5. Upload.
6. Open Serial Monitor and run `show` and `ptt_test`.

Keep the radio disconnected for the first upload and first `ptt_test`.
