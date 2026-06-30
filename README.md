# 9M2PJU ESP32 Fox Hunt Beacon

ESP32 firmware for an amateur radio fox hunting beacon using cheap handheld
walkie-talkies. The ESP32 handles timing, identification, audio tones,
configuration, battery monitoring, and push-to-talk control. The walkie-talkie
generates the actual RF signal.

The first firmware version is built with PlatformIO and the Arduino framework
for ESP32 boards. It is designed to key a handheld radio through a simple PTT
interface and feed CW or warble tone into the radio microphone input.

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

- PlatformIO project for common ESP32 boards.
- Configurable callsign and fox ID stored in ESP32 flash.
- Configurable transmit interval, transmit duration, and startup delay.
- CW ID generation using audio tone.
- Optional warble tone for the rest of the transmit window.
- PTT control with guard time before and after audio.
- PTT-only test command for checking radio keying before audio tests.
- Optional battery voltage measurement using a resistor divider.
- Status LED patterns for idle, transmit, and low battery.
- Test button for immediate transmission.
- Compile-time configuration file for default beacon behavior.
- Serial monitor configuration at 115200 baud.

Planned later features:

- Wi-Fi setup mode.
- Bluetooth serial configuration.
- Deep sleep between transmissions for longer battery life.
- Multiple fox profiles for event use.

## Supported ESP32 Boards

The design should work with most 3.3 V ESP32 development boards that expose
GPIO pins, ADC input, and USB programming.

Recommended boards:

| Board | Status | Notes |
| --- | --- | --- |
| ESP32 DevKit v1 / DOIT ESP32 DevKit | Recommended | Cheap, common, many GPIO pins, easy for prototypes. |
| Espressif ESP32-DevKitC | Recommended | Reliable reference-style development board. |
| ESP32-WROOM-32 boards | Recommended | Good default module family for this project. |
| ESP32-WROVER boards | Supported | Extra PSRAM is not required, but the board should work. |
| ESP32-S3 DevKit boards | Supported | Good modern option; pin mapping may differ. |
| ESP32-C3 boards | Limited | Usable for a simple beacon, but fewer GPIO pins and only one core. |
| TTGO / LilyGO ESP32 boards | Supported | Useful if they include display, battery charger, or LoRa hardware. |
| ESP32 LoRa boards | Conditional | Good for non-amateur LoRa experiments, but LoRa is not a normal VHF FM fox hunt transmitter. |

Tested PlatformIO build environments:

| Environment | Target board |
| --- | --- |
| `esp32dev` | Generic ESP32 Dev Module |
| `esp32doit-devkit-v1` | DOIT ESP32 DevKit v1 |
| `lolin32` | WEMOS LOLIN32 |
| `esp32-s3-devkitc-1` | ESP32-S3-DevKitC-1 |
| `esp32-c3-devkitm-1` | ESP32-C3-DevKitM-1 |

Boards to avoid for the first version:

- Boards without exposed GPIO pins.
- 5 V-only boards or accessories that drive ESP32 pins above 3.3 V.
- Very small boards without convenient ADC pins if battery monitoring is needed.
- Unknown transmitter boards without filtering, documentation, or legal output
  specifications.

## Radio Hardware Options

### Option A: ESP32 + Cheap Walkie-Talkie

This is the main target for the current firmware.

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

This is a possible standalone beacon design, but the current firmware is tuned
first for handheld radios.

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

Example schedule:

```text
00:00 - 00:05  PTT on, send "MOE" or callsign in CW
00:05 - 00:30  Send warble tone or intermittent tone
00:30 - 02:00  PTT off, sleep/idle
02:00 - 02:30  Repeat transmission
```

## Firmware

The repository includes a first working PlatformIO firmware in `src/main.cpp`.
It uses the Arduino framework for ESP32.

Source layout:

```text
platformio.ini
include/
  beacon_config.h
src/
  main.cpp
docs/
  wiring.md
  field-checklist.md
```

The main user configuration file is `include/beacon_config.h`. Edit that file
before building when you want the firmware image to contain your preferred
default callsign, fox ID, replay time, delay, timer, tone, PTT, battery, and pin
settings.

Implemented modules inside `main.cpp`:

- beacon scheduler
- CW keyer
- PWM audio tone generator
- PTT control
- battery monitor
- flash configuration storage
- serial command parser

Build the default ESP32 DevKit firmware:

```sh
pio run
```

Upload:

```sh
pio run -t upload
```

Open serial monitor:

```sh
pio device monitor -b 115200
```

Available PlatformIO environments:

```text
esp32dev
esp32doit-devkit-v1
lolin32
esp32-s3-devkitc-1
esp32-c3-devkitm-1
```

Example build for ESP32-C3:

```sh
pio run -e esp32-c3-devkitm-1
```

## Configuration

There are two ways to configure the beacon.

1. Edit `include/beacon_config.h` before uploading firmware.
2. Use Serial Monitor commands after uploading firmware.

Serial Monitor settings are saved in ESP32 flash memory and take priority over
the defaults in `include/beacon_config.h`. This is useful in the field because
you can change timing or ID without recompiling. If you want to restore the
values from `include/beacon_config.h`, open Serial Monitor and run:

```text
defaults
```

### Main Configuration File

Edit [include/beacon_config.h](include/beacon_config.h) for firmware defaults.

| Setting | Meaning | Default |
| --- | --- | --- |
| `DEFAULT_CALLSIGN` | Main callsign sent in CW at the start of TX. | `9M2PJU` |
| `DEFAULT_FOX_ID` | Fox/beacon label sent after the callsign. | `FOX1` |
| `DEFAULT_STARTUP_DELAY_SECONDS` | Wait time after power-on before the beacon schedule starts. Gives you time to hide the beacon. | `10` |
| `DEFAULT_TRANSMIT_SECONDS` | How long the radio transmits each cycle. | `30` |
| `DEFAULT_IDLE_SECONDS` | Replay/quiet time between transmissions. The radio is unkeyed during this time. | `90` |
| `DEFAULT_CW_WPM` | Morse speed for callsign and fox ID. | `12` |
| `DEFAULT_CW_TONE_HZ` | Audio tone frequency for CW. | `700` |
| `DEFAULT_WARBLE_ENABLED` | Sends alternating tone after CW until TX time ends. Use `0` for CW only. | `1` |
| `DEFAULT_WARBLE_LOW_HZ` | Low warble tone. | `700` |
| `DEFAULT_WARBLE_HIGH_HZ` | High warble tone. | `900` |
| `DEFAULT_WARBLE_STEP_MS` | How fast the warble alternates. | `350` |
| `DEFAULT_PTT_ACTIVE_LOW` | ESP32 GPIO active level. Use `0` for the recommended NPN/MOSFET/opto interface where GPIO HIGH keys the radio. | `0` |
| `DEFAULT_PTT_LEAD_MS` | Delay after keying PTT before audio starts. | `350` |
| `DEFAULT_PTT_TAIL_MS` | Delay before releasing PTT after audio stops. | `350` |
| `DEFAULT_BATTERY_ENABLED` | Enables low-battery cutoff when a divider is connected. | `0` |
| `DEFAULT_BATTERY_SCALE` | Voltage divider multiplier for battery readings. | `2.0` |
| `DEFAULT_LOW_BATTERY_VOLTAGE` | Voltage where firmware stops transmitting. | `3.40` |
| `PTT_PIN` | ESP32 pin that controls the walkie-talkie PTT circuit. | `25` |
| `AUDIO_PIN` | ESP32 PWM audio output pin. | `26` |
| `LED_PIN` | Status LED pin. | `2` |
| `BUTTON_PIN` | Test transmit button pin. | `0` |
| `BATTERY_PIN` | ADC pin for battery divider. | `34` |

### Timing Example

With these settings:

```c
#define DEFAULT_STARTUP_DELAY_SECONDS 300
#define DEFAULT_TRANSMIT_SECONDS 30
#define DEFAULT_IDLE_SECONDS 90
```

The beacon waits 5 minutes after power-on, transmits for 30 seconds, stays quiet
for 90 seconds, and then repeats. The full replay cycle is 120 seconds after the
startup delay.

### What The Beacon Sends

During each transmit window, the firmware sends:

```text
CALLSIGN in CW -> short gap -> FOX_ID in CW -> warble tone until TX timer ends
```

For example, with `DEFAULT_CALLSIGN` set to `9M2PJU` and `DEFAULT_FOX_ID` set to
`FOX1`, the radio sends `9M2PJU FOX1` in Morse code and then continues with the
warble tone until `DEFAULT_TRANSMIT_SECONDS` is finished.

If `DEFAULT_WARBLE_ENABLED` is `0`, it only sends the CW ID sequence and then
unkeys after the transmit timer completes.

### Serial Configuration

Open the serial monitor at 115200 baud. Settings are saved to ESP32 flash.

```text
show
test
ptt_test
defaults
reboot
set call 9M2PJU
set fox FOX1
set startup 300
set tx 30
set idle 90
set wpm 12
set tone 700
set warble on
set warble off
set ptt active_low
set ptt active_high
set battery on
set battery off
set battery_scale 2.0
set low_battery 3.4
```

Command meanings:

| Command | Meaning |
| --- | --- |
| `show` | Print current saved configuration. |
| `test` | Queue one immediate transmission. |
| `ptt_test` | Key only the PTT line for about 1.2 seconds with no audio. |
| `defaults` | Restore values from `include/beacon_config.h` and save them to flash. |
| `reboot` | Restart the ESP32. |
| `set call <text>` | Set the callsign sent in CW. |
| `set fox <text>` | Set the fox ID sent after the callsign. |
| `set startup <seconds>` | Set power-on hiding delay. |
| `set tx <seconds>` | Set transmit duration. |
| `set idle <seconds>` | Set quiet/replay delay between transmissions. |
| `set wpm <number>` | Set CW speed. |
| `set tone <hz>` | Set CW tone frequency. |
| `set warble on/off` | Enable or disable the warble tone. |
| `set ptt active_low/active_high` | Match the PTT interface circuit. |
| `set battery on/off` | Enable or disable low-battery protection. |
| `set battery_scale <number>` | Set battery divider multiplier. |
| `set low_battery <volts>` | Set low-battery cutoff voltage. |

## Default Pin Plan

These defaults are chosen for common ESP32 DevKit boards. ESP32-S3 and ESP32-C3
environments override some pins in `platformio.ini`.

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
- For the wiring in `docs/wiring.md`, keep `DEFAULT_PTT_ACTIVE_LOW` set to `0`
  or run `set ptt active_high`.
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
- Settings are stored in ESP32 flash and can be restored to compile-time
  defaults with `defaults`.
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
- PTT lead time: 300 ms.
- PTT tail time: 300 ms.

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

## Build Plan

### Phase 1: Minimal Beacon

- ESP32 controls one PTT pin. Done.
- ESP32 generates CW ID tone. Done.
- ESP32 generates optional warble tone. Done.
- Test using a handheld radio or dummy load setup.

### Phase 2: Configurable Beacon

- Store callsign and timing in flash. Done.
- Add serial command configuration. Done.
- Add battery voltage reading. Done.
- Add LED status patterns. Done.

### Phase 3: Field-Ready Beacon

- Add Wi-Fi setup mode for phone configuration.
- Add enclosure, switch, charge connector, and antenna connector.
- Add low-battery cutoff.
- Add field checklist and wiring diagram.
- Add a tested schematic for common Kenwood-style speaker-mic plugs.
- Add weatherproof field enclosure notes.

### Phase 4: Multi-Fox Event Support

- Store multiple fox profiles.
- Support unique tone/CW pattern per beacon.
- Add staggered start delay.
- Add optional GPS time or phone-based setup workflow.
- Add event mode for Fox 1 through Fox 5 timing patterns.

## Example Configuration

```text
callsign = 9M2PJU
fox_id = FOX1
frequency = configured on radio module or handheld radio
tx_duration_seconds = 30
idle_duration_seconds = 90
startup_delay_seconds = 300
cw_wpm = 12
cw_tone_hz = 700
ptt_active_level = LOW
low_battery_voltage = 3.4
```

## First Prototype Checklist

- ESP32 board can be programmed over USB.
- PTT output toggles correctly with an LED before connecting a radio.
- `ptt_test` keys and releases the PTT circuit reliably.
- Audio output produces the expected tone on a small amplified speaker or scope.
- Audio level is reduced before connecting to a transmitter.
- Radio keys and unkeys reliably.
- Beacon identifies correctly.
- Battery voltage reading is calibrated.
- RF output is tested into a dummy load before field use.
- Event frequency, power, ID, and timing are approved.

## Next Development Steps

1. Test on an ESP32 DevKit v1 with a cheap handheld radio interface.
2. Tune audio filter and trimpot values for common walkie-talkies.
3. Add example wiring photos or schematic.
4. Split firmware into modules once the prototype behavior is proven.
5. Add Wi-Fi setup mode.
6. Add support notes for SA818V/DRA818V style modules.
7. Add a simple web page to edit callsign, fox ID, TX time, replay delay, and
   battery cutoff from a phone.
8. Add optional deep sleep for long battery hunts.
9. Add release binaries for common ESP32 boards.
10. Add hardware-tested profiles for common Baofeng/Kenwood-style speaker-mic
    cables after confirming pinouts on real units.

## License

License not selected yet. Choose one before publishing firmware or hardware
files for reuse.
