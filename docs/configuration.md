# Configuration Guide

The beacon can be configured in two ways:

1. Edit `include/beacon_config.h` before building.
2. Use Serial Monitor commands after flashing.

Saved Serial Monitor settings are stored in ESP32 flash and override the
compile-time defaults. Run `defaults` in Serial Monitor to restore the values
from `include/beacon_config.h`.

## Recommended Basic Setup

For one training beacon:

```text
set call 9M2PJU
set fox MOE
set startup 300
set tx 30
set idle 90
set wpm 12
set tone 700
set warble on
```

For a multi-fox ARDF event, use a different fox identifier for each beacon:

| Beacon | Command |
| --- | --- |
| Fox 1 | `set fox MOE` |
| Fox 2 | `set fox MOI` |
| Fox 3 | `set fox MOS` |
| Fox 4 | `set fox MOH` |
| Fox 5 | `set fox MO5` |

Keep `set call ...` set to the licensed station or club callsign required by
your local rules.

## What The Beacon Transmits

Each transmit window sends:

```text
CALLSIGN in CW -> FOX_ID in CW -> optional warble tone
```

With the default settings, the radio sends:

```text
9M2PJU MOE
```

Then it warbles between 700 Hz and 900 Hz until the transmit timer ends.

## Compile-Time Defaults

Edit `include/beacon_config.h` when you want every fresh flash or `defaults`
command to return to your preferred setup.

| Setting | Meaning | Default |
| --- | --- | --- |
| `DEFAULT_CALLSIGN` | Callsign sent in CW. | `9M2PJU` |
| `DEFAULT_FOX_ID` | ARDF fox identifier sent after the callsign. | `MOE` |
| `DEFAULT_STARTUP_DELAY_SECONDS` | Delay after power-on before the schedule starts. | `10` |
| `DEFAULT_TRANSMIT_SECONDS` | Length of each transmit window. | `30` |
| `DEFAULT_IDLE_SECONDS` | Quiet time between transmissions. | `90` |
| `DEFAULT_CW_WPM` | Morse speed. | `12` |
| `DEFAULT_CW_TONE_HZ` | CW audio tone. | `700` |
| `DEFAULT_WARBLE_ENABLED` | Enables warble after the CW ID. | `1` |
| `DEFAULT_WARBLE_LOW_HZ` | Low warble frequency. | `700` |
| `DEFAULT_WARBLE_HIGH_HZ` | High warble frequency. | `900` |
| `DEFAULT_WARBLE_STEP_MS` | Warble step duration. | `350` |
| `DEFAULT_PTT_ACTIVE_LOW` | ESP32 GPIO active level for PTT. | `0` |
| `DEFAULT_PTT_LEAD_MS` | Delay after PTT before audio starts. | `350` |
| `DEFAULT_PTT_TAIL_MS` | Delay before releasing PTT after audio stops. | `350` |
| `DEFAULT_BATTERY_ENABLED` | Enables low-battery cutoff. | `0` |
| `DEFAULT_BATTERY_SCALE` | Battery divider multiplier. | `2.0` |
| `DEFAULT_LOW_BATTERY_VOLTAGE` | Voltage where TX stops. | `3.40` |

Default pins for classic ESP32 boards:

| Function | GPIO |
| --- | ---: |
| PTT output | 25 |
| Audio output | 26 |
| Status LED | 2 |
| Test button | 0 |
| Battery ADC | 34 |

ESP32-S3 and ESP32-C3 environments override some pins in `platformio.ini`.

## Serial Commands

Open Serial Monitor at 115200 baud and type commands followed by Enter.

| Command | Meaning |
| --- | --- |
| `show` | Print current saved configuration and command list. |
| `test` | Queue one complete beacon transmission. |
| `ptt_test` | Key PTT for about 1.2 seconds with no audio. |
| `defaults` | Restore compile-time defaults and save them to flash. |
| `reboot` | Restart the ESP32. |
| `set call <text>` | Set the CW callsign. |
| `set fox <text>` | Set the ARDF fox ID, such as `MOE` or `MOI`. |
| `set startup <seconds>` | Set startup hiding delay. |
| `set tx <seconds>` | Set transmit duration. |
| `set idle <seconds>` | Set quiet time between transmissions. |
| `set wpm <number>` | Set CW speed. |
| `set tone <hz>` | Set CW tone frequency. |
| `set warble on` | Enable warble after the CW ID. |
| `set warble off` | Disable warble after the CW ID. |
| `set warble_low <hz>` | Set low warble frequency. |
| `set warble_high <hz>` | Set high warble frequency. |
| `set warble_step <ms>` | Set how fast the warble alternates. |
| `set lead <ms>` | Set delay after PTT before audio. |
| `set tail <ms>` | Set delay before PTT release after audio. |
| `set ptt active_high` | GPIO HIGH keys the interface. |
| `set ptt active_low` | GPIO LOW keys the interface. |
| `set battery on` | Enable low-battery cutoff. |
| `set battery off` | Disable low-battery cutoff. |
| `set battery_scale <number>` | Set ADC divider multiplier. |
| `set low_battery <volts>` | Set low-battery cutoff voltage. |

The firmware constrains values to practical ranges. If a value is outside the
allowed range, it is clipped to the nearest allowed value.

## Timing Examples

Simple practice beacon:

```text
set startup 60
set tx 20
set idle 60
```

Classic two-minute cycle:

```text
set startup 300
set tx 30
set idle 90
```

Longer low-power training cycle:

```text
set startup 300
set tx 15
set idle 165
```

## Audio Setup

Start with:

```text
set wpm 12
set tone 700
set warble_low 700
set warble_high 900
set warble_step 350
```

If the received audio sounds harsh, distorted, or too wide, lower the physical
audio level going into the radio microphone input first. After that, try a
slower warble or lower tone frequencies.

## PTT Setup

For the recommended transistor, MOSFET, or optocoupler interface where GPIO HIGH
keys the radio:

```text
set ptt active_high
```

If your circuit keys the radio when the ESP32 output is LOW:

```text
set ptt active_low
```

Use `ptt_test` before connecting audio. The radio should key and then release
reliably every time.

## Battery Setup

Leave battery monitoring disabled until a voltage divider is installed and
calibrated:

```text
set battery off
```

After building the divider:

```text
set battery_scale 2.0
set low_battery 3.4
set battery on
show
```

Compare the voltage printed by `show` with a multimeter and adjust
`battery_scale` until it is close enough for field use.
