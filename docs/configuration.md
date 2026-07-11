# Configuration Guide

The beacon can be configured in three ways:

1. Edit `include/beacon_config.h` before building.
2. Use Serial Monitor commands after flashing.
3. Use the web admin UI from a phone or laptop over WiFi.

All three methods control the same settings. Serial Monitor and web UI settings
are stored in ESP32 flash and override the compile-time defaults. Run `defaults`
in Serial Monitor or click the Defaults button in the web UI to restore the
values from `include/beacon_config.h`.

Boards with a display also have an on-screen settings menu (double-click the
button) for quick on/off toggles. This is a convenience for field use and only
covers a subset of settings — full configuration requires the web UI or serial
monitor.

## Web Admin UI

The beacon hosts a WiFi access point with a captive portal. On boot, look for a
WiFi network named `FoxBeacon-XXXX` (last 4 hex of MAC address). Connect to it
from a phone or laptop — the configuration page should auto-open. If it does
not, browse to `http://192.168.4.1/`.

The web UI provides forms for all settings, plus buttons for test transmission,
PTT test, restore defaults, and reboot. No app or internet connection is needed.

The web UI works on all ESP32 boards. Boards with OLED or TFT screens also show
a live status display with callsign, fox ID, mode, state, timing, battery, and
AP IP address.

The WiFi AP can be turned on or off from the on-screen settings menu or the web
UI. When off, the WiFi radio is disabled to save power.

The AP also has an **auto-off timeout** (default 10 minutes). If no phone or
laptop is connected and no web requests are received for the timeout period, the
AP shuts down automatically to save power. Set the timeout to 0 to disable
auto-off (AP stays on indefinitely). Configure it via the web UI, serial
monitor (`set wifi_ap_timeout 10`), or it defaults to 10 minutes. To turn the AP
back on after auto-off, use the on-screen menu (double-click the button, select
WiFi AP, toggle ON) or the serial command `set wifi_ap on`.

## Recommended Basic Setup

For a standard IARU 5-fox event, the defaults already match the standard cycle
(60 s TX, 240 s idle, warble off, fox sync on). You only need to set the
callsign and fox ID on each beacon:

```text
set call 9M2PJU
set fox MOE
```

For a multi-fox ARDF event, use a different fox identifier for each beacon.
With fox sync enabled (the default), the startup delay is calculated
automatically from the fox ID, so all five beacons use the same timing:

| Beacon | Command | Auto startup delay |
| --- | --- | ---: |
| Fox 1 | `set fox MOE` | 0 s |
| Fox 2 | `set fox MOI` | 60 s |
| Fox 3 | `set fox MOS` | 120 s |
| Fox 4 | `set fox MOH` | 180 s |
| Fox 5 | `set fox MO5` | 240 s |

Power all five beacons on at roughly the same time and they will take turns
automatically in the standard round-robin order.

For a finish-line beacon (MO6) on a separate frequency:

```text
set mode beacon
set fox MO6
set beacon_id 60
```

Keep `set call ...` set to the licensed station or club callsign required by
your local rules.

## What The Beacon Transmits

Each transmit window sends:

```text
CALLSIGN in CW -> FOX_ID in CW -> steady carrier (or warble if enabled)
```

With the default settings, the radio sends:

```text
9M2PJU MOE
```

Then it holds a steady carrier until the transmit timer ends. If warble is
enabled, it alternates between 700 Hz and 900 Hz instead.

## Compile-Time Defaults

Edit `include/beacon_config.h` when you want every fresh flash or `defaults`
command to return to your preferred setup.

| Setting | Meaning | Default |
| --- | --- | --- |
| `DEFAULT_CALLSIGN` | Callsign sent in CW. | `9M2PJU` |
| `DEFAULT_FOX_ID` | ARDF fox identifier sent after the callsign. | `MOE` |
| `DEFAULT_STARTUP_DELAY_SECONDS` | Delay after power-on before the schedule starts. Overridden by fox sync when enabled. | `10` |
| `DEFAULT_TRANSMIT_SECONDS` | Length of each transmit window. | `60` |
| `DEFAULT_IDLE_SECONDS` | Quiet time between transmissions. | `240` |
| `DEFAULT_FOX_SYNC_ENABLED` | Auto-derive startup delay from fox ID for IARU round-robin. | `1` |
| `DEFAULT_BEACON_MODE` | Continuous beacon (MO6 finish) mode. | `0` |
| `DEFAULT_BEACON_ID_INTERVAL_SECONDS` | CW ID repeat interval in continuous beacon mode. | `60` |
| `DEFAULT_CW_WPM` | Morse speed. | `12` |
| `DEFAULT_CW_TONE_HZ` | CW audio tone. | `700` |
| `DEFAULT_WARBLE_ENABLED` | Enables warble after the CW ID. Disabled for IARU standard. | `0` |
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
| `set mode fox\|beacon` | Switch between scheduled fox mode and continuous beacon (MO6) mode. |
| `set fox_sync on\|off` | Enable or disable auto startup delay from fox ID. |
| `set beacon_id <seconds>` | Set CW ID repeat interval for continuous beacon mode. |
| `set startup <seconds>` | Set startup hiding delay (overridden by fox sync when enabled). |
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

Standard IARU 5-fox event (the default):

```text
set fox_sync on
set tx 60
set idle 240
set warble off
```

Simple practice beacon with faster cycling:

```text
set fox_sync off
set startup 60
set tx 20
set idle 60
set warble on
```

Classic two-minute training cycle:

```text
set fox_sync off
set startup 300
set tx 30
set idle 90
```

Longer low-power training cycle:

```text
set fox_sync off
set startup 300
set tx 15
set idle 165
```

Finish-line continuous beacon (MO6) on a separate frequency:

```text
set mode beacon
set fox MO6
set beacon_id 60
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
