# 9M2PJU ESP32 Fox Hunt Beacon

<p align="center">
  <img src="images/fox-beacon-logo.png" alt="9M2PJU ESP32 Fox Hunt Beacon" width="600">
</p>

ESP32 firmware for an amateur radio fox hunting beacon using cheap handheld
walkie-talkies. The ESP32 handles timing, identification, audio tones,
configuration, battery monitoring, and push-to-talk control. The walkie-talkie
generates the actual RF signal.

The firmware is built with PlatformIO and the Arduino framework for ESP32
boards. It is designed to key a handheld radio through a simple PTT interface
and feed CW or warble tone into the radio microphone input.

## Flash The Firmware

Install the firmware directly from your browser — no software install, no
build tools, no command line.

**Web Flasher: <https://fox.hamradio.my>**

Requirements:

- Google Chrome, Microsoft Edge, or Opera (Web Serial API required).
  Firefox and Safari are not supported.
- USB cable with data lines (many cheap cables are charge-only).
- USB-UART driver for your board (CP2102, CH340, or FT232 depending on board).

Open the web flasher, select your board from the list, click **Install
Firmware**, and connect your board via USB when prompted. The flasher downloads
the correct prebuilt firmware for your board and writes it to the ESP32 over
USB through the browser.

After flashing, the board boots into WiFi AP mode. Connect to the
`9M2PJU-Fox-XXXX` WiFi network from a phone or laptop and browse to
`http://10.0.0.8/` to configure callsign, fox ID, timing, PTT, audio, and
other settings.

## Documentation

This README is the project overview. The detailed user guides are in the
[`docs/`](docs/) folder. New users should read the docs before wiring a radio or
transmitting, because the beacon needs correct callsign, ARDF fox ID, PTT
polarity, audio level, and local radio-rule setup.

Recommended reading order:

1. [Understanding The Beacon](docs/understanding.md)
2. [Configuration Guide](docs/configuration.md)
3. [Heltec Board Support](docs/heltec-boards.md)
4. [LilyGO Board Support](docs/lilygo-boards.md)
5. [Wiring Guide](docs/wiring.md)
6. [Field Checklist](docs/field-checklist.md)
7. [Troubleshooting](docs/troubleshooting.md)

Quick guide map:

| Guide | Use it for |
| --- | --- |
| [Understanding The Beacon](docs/understanding.md) | ARDF basics, what the beacon sends, and how the ESP32/radio parts work together. |
| [Configuration Guide](docs/configuration.md) | Callsign, ARDF fox IDs, timing, warble, PTT, battery, and Serial Monitor commands. |
| [Heltec Board Support](docs/heltec-boards.md) | Pin notes for popular Heltec ESP32 boards. |
| [LilyGO Board Support](docs/lilygo-boards.md) | Pin notes for popular LilyGO and TTGO ESP32 boards. |
| [Wiring Guide](docs/wiring.md) | PTT/audio interface notes for cheap handheld radios. |
| [Field Checklist](docs/field-checklist.md) | Pre-hunt bench checks, radio setup, power setup, deployment, and recovery. |
| [Troubleshooting](docs/troubleshooting.md) | Flashing, serial, PTT, audio, battery, timing, and RF problems. |

## Can This Be Made With ESP32 Boards?

Yes. An ESP32 board is a good controller for a fox hunt beacon because it has
accurate enough timing, low-power sleep modes, Wi-Fi/Bluetooth for
configuration, GPIO for PTT control, ADC input for battery sensing, and enough
processing power to generate CW or audio tones.

However, an ESP32 board by itself is not a clean or legal VHF/UHF amateur radio
transmitter. For this project, the ESP32 controls a cheap handheld walkie-talkie
or amateur handheld radio. That keeps the RF side cleaner, cheaper, and easier
to test.

Recommended transmitter approaches:

- VHF FM module such as SA818V, DRA818V, or compatible 144 MHz module.
- UHF FM module such as SA818U, DRA818U, or compatible 430 MHz module.
- Cheap handheld walkie-talkie controlled through PTT and audio input.
- External amateur handheld radio controlled through PTT and audio input.
- Other amateur-band transmitter modules, if they meet local rules and spectral
  purity requirements.

Always follow your local amateur radio regulations, band plans, identification
requirements, power limits, and event safety rules.

## Project Goals

- Build a portable hidden transmitter controller for amateur radio direction
  finding.
- Support timed beacon cycles, for example 30 seconds on and 90 seconds off.
- Send a callsign or fox ID using Morse code, voice/audio tone, or both.
- Control transmitter PTT from an ESP32 GPIO pin.
- Feed audio tones from the ESP32 to the transmitter microphone/audio input.
- Support battery-powered operation in the field.
- Provide simple configuration without recompiling firmware.
- Keep the hardware easy to build from common ESP32 development boards.

## Firmware Features

- Configurable callsign and fox ID stored in ESP32 flash.
- IARU standard 5-fox timing defaults (60 s TX, 240 s idle, 300 s cycle).
- Fox-slot synchronization: auto-derive startup delay from fox ID for round-robin.
- Continuous beacon mode (MO6 finish-line transmitter) on a separate frequency.
- CW ID generation using audio tone.
- Optional warble tone for the rest of the transmit window (disabled by default).
- PTT control with guard time before and after audio.
- PTT-only test command for checking radio keying before audio tests.
- Optional battery voltage measurement using a resistor divider.
- Status LED patterns for idle, transmit, and low battery.
- Test button for immediate transmission.
- Serial monitor configuration at 115200 baud.
- Web admin UI: WiFi AP with captive portal for phone/laptop configuration.
- On-screen status display for boards with OLED, TFT, or E-Ink screens.
- Display eco mode (auto-off after 15 s) for battery saving.

Planned later features:

- Bluetooth serial configuration.
- Deep sleep between transmissions for longer battery life.
- Multiple fox profiles for event use.

## Supported ESP32 Boards

The firmware supports 32 ESP32 board environments, all available through the
web flasher at <https://fox.hamradio.my>.

| Board | Chip | Display |
| --- | --- | --- |
| Generic ESP32 DevKit | ESP32 | No display |
| ESP32 DOIT DevKit V1 | ESP32 | No display |
| WEMOS LOLIN32 | ESP32 | No display |
| Generic ESP32-S3 DevKitC-1 | ESP32-S3 | No display |
| Generic ESP32-C3 DevKitM-1 | ESP32-C3 | No display |
| Heltec WiFi Kit 32 (V1/V2/V3) | ESP32 / S3 | OLED 128x64 |
| Heltec WiFi LoRa 32 (V1/V2/V3) | ESP32 / S3 | OLED 128x64 |
| Heltec Wireless Stick | ESP32 | OLED 128x64 |
| Heltec Wireless Stick Lite | ESP32 | OLED 128x64 |
| Heltec Wireless Stick Lite V3 | ESP32-S3 | OLED 128x64 |
| Heltec Wireless Tracker | ESP32-S3 | TFT 160x80 |
| Heltec Vision Master T190 | ESP32-S3 | TFT 170x320 |
| Heltec Wireless Paper | ESP32-S3 | E-Ink 250x122 |
| Heltec Vision Master E213 | ESP32-S3 | E-Ink 250x122 |
| Heltec Vision Master E290 | ESP32-S3 | E-Ink 296x128 |
| Heltec Capsule Sensor V3 | ESP32-S3 | No display |
| LilyGO T-Display | ESP32 | TFT 135x240 |
| LilyGO T-Display S3 | ESP32-S3 | TFT 170x320 |
| LilyGO T3-S3 | ESP32-S3 | No display |
| TTGO LoRa32 V1 | ESP32 | OLED 128x64 |
| TTGO LoRa32 V2 | ESP32 | OLED 128x64 |
| TTGO LoRa32 V2.1.6 | ESP32 | OLED 128x64 |
| TTGO T-Beam | ESP32 | OLED 128x64 |
| TTGO T-OI Plus | ESP32-C3 | No display |
| TTGO T-Watch | ESP32 | TFT 240x240 |
| TTGO T1 | ESP32 | No display |
| TTGO T7 V1.3 Mini32 | ESP32 | No display |
| TTGO T7 V1.4 Mini32 | ESP32 | No display |

Boards to avoid:

- Boards without exposed GPIO pins.
- 5 V-only boards or accessories that drive ESP32 pins above 3.3 V.
- Very small boards without convenient ADC pins if battery monitoring is needed.
- Unknown transmitter boards without filtering, documentation, or legal output
  specifications.

## Radio Hardware Options

### Option A: ESP32 + Cheap Walkie-Talkie

This is the main target for the firmware.

Typical parts:

- ESP32 development board.
- Cheap handheld radio or amateur handheld transceiver.
- Speaker-mic cable or accessory plug for the radio.
- NPN transistor, MOSFET, reed relay, or optocoupler for PTT.
- Audio coupling capacitor, RC filter, and trimpot for mic input level.
- USB power bank or small field battery.
- Optional battery divider into ESP32 ADC.
- Status LED and optional push button.

Basic signal flow:

```text
ESP32 GPIO 25  -> PTT transistor/opto -> walkie-talkie PTT
ESP32 GPIO 26  -> RC filter/trimpot   -> walkie-talkie microphone input
ESP32 GPIO 0   -> test button         -> immediate test transmission
ESP32 GPIO 34  -> battery divider     -> battery monitor
```

See [docs/wiring.md](docs/wiring.md) for wiring notes.

### Option B: ESP32 + VHF FM Transmitter Module

This is a possible standalone beacon design, but the firmware is tuned first
for handheld radios.

Typical parts:

- ESP32 development board.
- SA818V, DRA818V, or compatible VHF FM module.
- Low-pass or band-pass filtering appropriate for the band.
- Quarter-wave antenna or small directional/event-approved antenna.
- 3.7 V Li-ion, 2-cell Li-ion, LiFePO4, or USB power bank supply.
- Voltage regulator sized for the transmitter current.
- PTT transistor or logic-level interface.
- Audio coupling capacitor and level-setting resistor divider.
- Battery voltage divider to ESP32 ADC.
- Status LED and optional push button.

Basic signal flow:

```text
ESP32 GPIO       -> PTT interface      -> RF module PTT
ESP32 DAC/PWM    -> audio filter/level -> RF module MIC/audio input
Battery divider  -> ESP32 ADC          -> battery monitor
RF module output -> filter             -> antenna
```

### Option C: ESP32 + Handheld Amateur Radio

This is often the easiest legal and clean RF path because the handheld radio is
already type-built for amateur FM operation.

The ESP32 provides:

- PTT switching through an optocoupler, transistor, or radio-specific cable.
- Audio tone into the radio microphone input.
- Timing and ID logic.

This option is good for first tests because the RF side is already solved.

### Option D: ESP32 + LoRa Module

This is useful for experiments, telemetry, or training games, but it is not the
classic 2 m or 70 cm FM fox hunting beacon. Use this only when the event rules
and local regulations allow it.

## How It Works

The beacon is a timed transmitter controller.

1. On boot, the ESP32 loads saved configuration from flash memory.
2. It waits for an optional startup delay so the operator can hide the beacon.
3. It enters the beacon schedule loop.
4. During the idle window, the transmitter is off and the ESP32 may sleep.
5. Before transmission, the ESP32 enables PTT and waits a short guard time.
6. The ESP32 generates the programmed audio pattern.
7. The transmitter sends the tone, CW ID, or voice/audio sequence.
8. The ESP32 releases PTT after a short tail delay.
9. Battery voltage is checked and status LEDs are updated.
10. The cycle repeats until the beacon is powered off or the battery limit is
    reached.

Example schedule (IARU 5-fox cycle, fox 1 / MOE):

```text
00:00 - 01:00  PTT on, send "9M2PJU MOE" in CW, then steady carrier
01:00 - 05:00  PTT off, idle (foxes 2-5 take their turns)
05:00 - 06:00  Repeat transmission
```

With fox sync enabled, fox 2 (MOI) starts at 01:00, fox 3 (MOS) at 02:00, fox 4
(MOH) at 03:00, and fox 5 (MO5) at 04:00. All five beacons share one frequency
and take turns in the standard round-robin.

## Configuration

There are two ways to configure the beacon after flashing:

1. Use the WiFi web admin UI at `http://10.0.0.8`.
2. Use Serial Monitor commands at 115200 baud.

Both methods save settings to ESP32 flash memory and persist across reboots. Run
`defaults` over serial to restore the factory defaults.

### Web Admin UI

The beacon hosts a WiFi access point with a captive portal web UI for
phone/laptop configuration. This works on all ESP32 boards (all have WiFi).

On boot, the ESP32 starts a WiFi AP named `9M2PJU-Fox-XXXX` (last 4 hex of MAC).
Connect to it from a phone or laptop and the captive portal should auto-open the
configuration page. If it does not, browse to `http://10.0.0.8/`.

The web UI lets you:

- View current status (state, battery, fox slot, AP name, IP)
- Edit all settings: callsign, fox ID, mode, fox sync, timing, CW, warble, PTT,
  battery, WiFi AP on/off, AP auto-off timeout, display eco mode
- Save settings to flash (same as serial `set` commands)
- Trigger a test transmission or PTT test
- Restore factory defaults
- Reboot the ESP32

The web UI is served on all boards. No router or internet connection is needed.
The AP has no password by default so it is open for easy field access.

The WiFi AP can be turned on or off from the on-screen settings menu or the web
UI. When off, the WiFi radio is disabled to save power in the field.

The AP has an **auto-off timeout** (default 10 minutes). If no client is
connected and no web requests are received for the timeout period, the AP shuts
down automatically to save power. Set to 0 to keep the AP on indefinitely. To
turn it back on, use the on-screen menu (WiFi AP toggle) or `set wifi_ap on`.

### Serial Configuration

Open the serial monitor at 115200 baud. Settings are saved to ESP32 flash.

```text
show
test
ptt_test
defaults
reboot
set call 9M2PJU
set fox MOE
set mode fox
set mode beacon
set fox_sync on
set fox_sync off
set beacon_id 60
set startup 300
set tx 60
set idle 240
set wpm 12
set tone 700
set warble on
set warble off
set warble_low 700
set warble_high 900
set warble_step 350
set lead 350
set tail 350
set ptt active_low
set ptt active_high
set battery on
set battery off
set battery_scale 2.0
set low_battery 3.4
set wifi_ap on
set wifi_ap off
set wifi_ap_timeout 10
set eco_mode on
set eco_mode off
```

Command meanings:

| Command | Meaning |
| --- | --- |
| `show` | Print current saved configuration. |
| `test` | Queue one immediate transmission. |
| `ptt_test` | Key only the PTT line for about 1.2 seconds with no audio. |
| `defaults` | Restore factory defaults and save them to flash. |
| `reboot` | Restart the ESP32. |
| `set call <text>` | Set the callsign sent in CW. |
| `set fox <text>` | Set the ARDF fox identifier sent after the callsign, such as `MOE`, `MOI`, `MOS`, `MOH`, or `MO5`. |
| `set mode fox\|beacon` | Switch between scheduled fox mode and continuous beacon (MO6 finish) mode. |
| `set fox_sync on\|off` | Enable or disable auto startup delay from fox ID for IARU round-robin. |
| `set beacon_id <seconds>` | Set CW ID repeat interval for continuous beacon mode. |
| `set startup <seconds>` | Set power-on hiding delay (overridden by fox sync when enabled). |
| `set tx <seconds>` | Set transmit duration. |
| `set idle <seconds>` | Set quiet/replay delay between transmissions. |
| `set wpm <number>` | Set CW speed. |
| `set tone <hz>` | Set CW tone frequency. |
| `set warble on/off` | Enable or disable the warble tone. |
| `set warble_low <hz>` | Set low warble frequency. |
| `set warble_high <hz>` | Set high warble frequency. |
| `set warble_step <ms>` | Set how fast the warble alternates. |
| `set lead <ms>` | Set delay after PTT before audio starts. |
| `set tail <ms>` | Set delay before PTT releases after audio stops. |
| `set ptt active_low/active_high` | Match the PTT interface circuit. |
| `set battery on/off` | Enable or disable low-battery protection. |
| `set battery_scale <number>` | Set battery divider multiplier. |
| `set low_battery <volts>` | Set low-battery cutoff voltage. |
| `set wifi_ap on/off` | Turn WiFi AP on or off. |
| `set wifi_ap_timeout <minutes>` | AP auto-off timeout (0 = never). |
| `set eco_mode on/off` | Enable or disable display eco mode. |

### Timing Example

With the default IARU settings and fox sync enabled, the beacon transmits for
60 seconds, stays quiet for 240 seconds, and repeats. The full cycle is 300
seconds (5 minutes). For fox 1 (`MOE`), the beacon starts immediately after
power-on. For fox 2 (`MOI`), the startup delay is automatically set to 60
seconds so it takes the second slot in the round-robin. Fox 3 starts at 120 s,
fox 4 at 180 s, fox 5 at 240 s. All five beacons share one frequency and take
turns.

### What The Beacon Sends

During each transmit window, the firmware sends:

```text
CALLSIGN in CW -> short gap -> FOX_ID in CW -> steady carrier until TX timer ends
```

For example, with the callsign set to `9M2PJU` and the fox ID set to `MOE`, the
radio sends `9M2PJU MOE` in Morse code and then holds a steady carrier until the
transmit timer ends. This matches the IARU ARDF standard signal format.

If warble is enabled, it sends the CW ID sequence and then alternates a warble
tone between the low and high warble frequencies until the TX timer ends. This
is non-standard but useful for training.

ARDF means Amateur Radio Direction Finding. In a typical ARDF event, several
hidden transmitters, or "foxes", take turns transmitting short Morse
identifiers so competitors can tell which fox they are hearing. The conventional
2 m/80 m ARDF identifiers are `MOE`, `MOI`, `MOS`, `MOH`, and `MO5`.

| Fox | CW identifier |
| --- | --- |
| 1 | `MOE` |
| 2 | `MOI` |
| 3 | `MOS` |
| 4 | `MOH` |
| 5 | `MO5` |

For a single training beacon, `MOE` is a good default. For multi-fox events,
program each beacon with a different identifier and timing plan so hunters can
separate the transmitters. Keep the callsign set to the licensed station
callsign required for your local amateur radio identification rules.

## On-Screen Status Display

Boards with an OLED, TFT, or E-Ink screen show a live status display.

### Startup Screen

On boot, the display shows a startup splash for 3 seconds:

```
9M2PJU Fox
────────────────
v1.1.0
Starting...
```

After 3 seconds, it transitions to the status screen.

### Status Screen

The status screen shows:

- Callsign and fox ID
- Mode (FOX or BEACON)
- Current state (STARTUP, IDLE, TX, BEACON, LOWBAT)
- Timing (TX and idle seconds)
- Battery voltage (if battery monitoring is enabled)
- Web admin AP IP address (if WiFi AP is enabled)

The display updates once per second.

### Settings Menu (On-Screen)

Double-click the button on the status screen to open the settings menu. The menu
provides quick on/off toggles for field use without needing a phone or laptop:

| Item | Description |
| --- | --- |
| WiFi AP | Turn WiFi AP / web admin on or off |
| Warble | Enable/disable warble tone |
| Fox Sync | Enable/disable fox slot synchronization |
| Battery | Enable/disable battery monitoring |
| Mode | Toggle between FOX and BEACON mode |
| Eco Disp | Enable/disable display eco mode (auto-off after 15s) |
| Exit | Return to status screen |

Menu navigation:

- **Single click**: Move to next item
- **Double click**: Toggle selected item
- **Long hold (2s)**: Exit menu
- Menu auto-exits after 30 seconds of inactivity

Full configuration (callsign, fox ID, timing, CW speed, PTT, warble frequencies,
battery scale, etc.) is only available through the WiFi web admin UI. The
on-screen menu is limited to on/off toggles for quick field adjustments.

### Display Eco Mode

When eco mode is enabled, the display turns off after 15 seconds of inactivity
to save power. Any button press wakes the display. This is useful for extended
field operations where battery life matters more than constant status visibility.

Supported display boards:

| Board | Display type | Library |
| --- | --- | --- |
| Heltec WiFi Kit 32 (V1/V2) | OLED SSD1306 128x64 | U8g2 |
| Heltec WiFi Kit 32 V3 | OLED SSD1306 128x64 | U8g2 |
| Heltec WiFi LoRa 32 (V1/V2/V3) | OLED SSD1306 128x64 | U8g2 |
| Heltec Wireless Stick / Stick Lite | OLED SSD1306 128x64 | U8g2 |
| Heltec Wireless Stick Lite V3 | OLED SSD1306 128x64 | U8g2 |
| TTGO LoRa32 V1 | OLED SSD1306 128x64 | U8g2 |
| TTGO LoRa32 V2 | OLED SSD1306 128x64 | U8g2 |
| TTGO LoRa32 V2.1.6 | OLED SSD1306 128x64 | U8g2 |
| TTGO T-Beam | OLED SSD1306 128x64 | U8g2 |
| Heltec Wireless Tracker | TFT ST7735 160x80 | TFT_eSPI |
| Heltec Vision Master T190 | TFT ST7789 170x320 | TFT_eSPI |
| LilyGO T-Display | TFT ST7789 135x240 | TFT_eSPI |
| LilyGO T-Display S3 | TFT ST7789 170x320 | TFT_eSPI |
| TTGO T-Watch | TFT ST7789 240x240 | TFT_eSPI |
| Heltec Wireless Paper | E-Ink SSD1680 250x122 | GxEPD2 |
| Heltec Vision Master E213 | E-Ink SSD1680 250x122 | GxEPD2 |
| Heltec Vision Master E290 | E-Ink SSD1680 296x128 | GxEPD2 |

Boards without displays (ESP32 DevKit, ESP32-S3 DevKit, ESP32-C3, TTGO T1, T7,
T-OI Plus, T3-S3, Heltec Capsule Sensor V3) still get the web admin UI — they
just have no screen.

## Default Pin Plan

These defaults are chosen for common ESP32 DevKit boards. ESP32-S3 and ESP32-C3
boards use different default pins because those chips expose different ADC and
boot-safe pins.

| Function | Default GPIO | Notes |
| --- | ---: | --- |
| PTT output | GPIO 25 | Drives transistor, optocoupler, or radio PTT input. |
| Audio tone output | GPIO 26 | PWM audio through RC filter, capacitor, and level pot. |
| Status LED | GPIO 2 | Common onboard LED on many ESP32 boards. |
| Test button | GPIO 0 | Boot button on many boards; avoid holding during reset. |
| Battery ADC | GPIO 34 | Input-only ADC pin on classic ESP32. |

Important pin notes:

- Use 3.3 V logic only on ESP32 GPIO pins.
- Do not connect transmitter audio or PTT lines directly unless the voltage
  levels are known.
- Use a transistor, optocoupler, or level shifting where needed.
- For the wiring in [docs/wiring.md](docs/wiring.md), use `set ptt active_high`.
- Avoid ESP32 boot strapping pins for critical external circuits unless the
  design is tested carefully.

## Reliability Notes

This project is intended to be practical for amateur radio direction finding,
but the radio interface must be built carefully.

Firmware reliability features:

- PTT is forced off during boot and again after saved settings are loaded.
- Transmit duration, idle duration, tone frequency, CW speed, and battery
  thresholds are constrained to sane ranges.
- `ptt_test` verifies the keying circuit without generating audio.
- `test` sends one complete beacon cycle on demand.
- Settings are stored in ESP32 flash and can be restored to factory defaults
  with `defaults`.
- Optional low-battery cutoff halts transmission instead of sending a weak or
  unstable signal.

Hardware reliability rules:

- Test PTT with an LED or multimeter before connecting the radio.
- Test RF into a dummy load or the lowest practical power before field use.
- Keep the audio level low and increase it slowly while listening on another
  receiver.
- Use strain relief on the speaker-mic cable.
- Keep RF, audio, and ESP32 power wiring short and tidy.
- Put the beacon in an enclosure with a physical power switch.
- Follow the [field checklist](docs/field-checklist.md) before hiding the unit.

## Audio Output

The ESP32 firmware generates PWM audio using LEDC. Feed this through a small RC
filter, coupling capacitor, and trimpot before the walkie-talkie microphone
input.

Suggested starting values:

- CW tone: 700 Hz.
- Warble tone: alternating 700 Hz and 900 Hz.
- CW speed: 12 WPM.
- PTT lead time: 350 ms.
- PTT tail time: 350 ms.

Use `test` to make one immediate transmission. The boot button on many ESP32
boards also queues a test transmission.

## Power Plan

Typical field power options:

- USB power bank into ESP32 USB and a separate transmitter supply.
- 1-cell Li-ion with boost regulator if the transmitter supports it.
- 2-cell Li-ion with buck regulator for ESP32 and transmitter.
- LiFePO4 battery for simpler voltage range and safer field handling.

Design reminders:

- Size the regulator for transmitter current, not only ESP32 current.
- Keep RF module power wiring short and decoupled.
- Add reverse-polarity and fuse protection for field use.
- Measure battery voltage through a resistor divider into an ADC pin.
- Calibrate ADC readings in firmware before trusting low-battery cutoff.

## Safety And Legal Notes

- Transmit only on frequencies, modes, and power levels allowed by your license
  and local rules.
- Identify the station as required.
- Use suitable filtering and a known-good antenna or dummy load during testing.
- Do not test into an antenna indoors at high power.
- Keep transmit power low for close-range events.
- Make sure the beacon can be shut down quickly.
- Label the beacon with owner contact information for public-area events.

## Example Configuration

```text
callsign = 9M2PJU
fox_id = MOE
frequency = configured on radio module or handheld radio
tx_duration_seconds = 60
idle_duration_seconds = 240
startup_delay_seconds = 10 (overridden by fox_sync)
fox_sync = on
beacon_mode = off
cw_wpm = 12
cw_tone_hz = 700
warble = off
ptt_active_level = HIGH
ptt_lead_ms = 350
ptt_tail_ms = 350
low_battery_voltage = 3.4
```

## First Prototype Checklist

- ESP32 board is flashed with the correct firmware from the web flasher.
- PTT output toggles correctly with an LED before connecting a radio.
- `ptt_test` keys and releases the PTT circuit reliably.
- Audio output produces the expected tone on a small amplified speaker or scope.
- Audio level is reduced before connecting to a transmitter.
- Radio keys and unkeys reliably.
- Beacon identifies correctly.
- Battery voltage reading is calibrated.
- RF output is tested into a dummy load before field use.
- Event frequency, power, ID, and timing are approved.

## License

This project is licensed under the GNU General Public License v3.0 or later
(`GPL-3.0-or-later`). See [LICENSE](LICENSE) for the full license text.
