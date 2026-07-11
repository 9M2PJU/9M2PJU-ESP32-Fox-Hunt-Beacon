# Understanding The Beacon

This project turns an ESP32 into a controller for an amateur radio fox hunt
beacon. The ESP32 does not generate the RF signal by itself. It controls a
handheld radio or transmitter module by keying PTT and feeding audio into the
microphone input.

## What The System Does

The beacon repeats a simple cycle:

```text
startup delay -> transmit window -> idle window -> transmit window -> ...
```

During the transmit window, the firmware:

1. Keys the radio PTT line.
2. Waits for the radio to settle.
3. Sends the configured callsign in Morse code.
4. Sends the configured fox identifier in Morse code.
5. Holds a steady carrier (or optional warble tone) until the transmit window
   ends.
6. Stops audio and releases PTT.

The default on-air message is:

```text
9M2PJU MOE
```

`9M2PJU` is the station callsign. `MOE` is the ARDF Fox 1 identifier.

## What ARDF Means

ARDF means Amateur Radio Direction Finding. In an ARDF event, competitors use
receivers and directional antennas to find hidden transmitters. Each hidden
transmitter is usually called a fox.

Common ARDF events use up to five foxes. Each fox sends a short Morse identifier
so hunters can tell which transmitter they are hearing:

| Fox | Morse identifier | Typical use |
| --- | --- | --- |
| 1 | `MOE` | First or only training beacon |
| 2 | `MOI` | Second event beacon |
| 3 | `MOS` | Third event beacon |
| 4 | `MOH` | Fourth event beacon |
| 5 | `MO5` | Fifth event beacon |

For one practice beacon, use `MOE`. For a multi-fox event, set each beacon to a
different identifier and give each beacon an event-specific timing plan.

## IARU ARDF Event Standard

The IARU Region 1 ARDF rules define a standard 5-fox cycle used in sanctioned
competitions. This firmware follows those conventions by default.

### Timing

Five foxes share a single frequency and take turns transmitting in a strict
round-robin:

```text
Fox 1 (MOE)  TX 0:00 - 1:00
Fox 2 (MOI)  TX 1:00 - 2:00
Fox 3 (MOS)  TX 2:00 - 3:00
Fox 4 (MOH)  TX 3:00 - 4:00
Fox 5 (MO5)  TX 4:00 - 5:00
Fox 1 (MOE)  TX 5:00 - 6:00  (cycle repeats)
```

Each fox transmits for **60 seconds**, then waits **240 seconds** while the
other four foxes take their turns. The full cycle is **300 seconds** (5
minutes). The firmware defaults match this: `tx=60`, `idle=240`.

### Fox-Slot Synchronization

When fox-slot sync is enabled (the default), the firmware derives the fox
number from the fox ID and automatically sets the startup delay so that five
beacons powered on at roughly the same time fall into the correct round-robin
order:

| Fox ID | Fox number | Auto startup delay |
| --- | ---: | ---: |
| `MOE` | 1 | 0 s |
| `MOI` | 2 | 60 s |
| `MOS` | 3 | 120 s |
| `MOH` | 4 | 180 s |
| `MO5` | 5 | 240 s |

This means you can program all five beacons with the same timing (`tx=60`,
`idle=240`, `fox_sync on`) and only differ in the fox ID. Power them on at the
same time and they will take turns automatically.

Fox-slot sync only works with the five standard ARDF fox IDs. If you use a
custom fox ID, the firmware falls back to the manual `startup` delay. Disable
sync with `set fox_sync off` if you prefer manual control.

### Signal Format

Per IARU convention, each fox sends its CW Morse identifier followed by a
steady carrier for the remainder of its 60-second window. The warble tone is
disabled by default for standard compliance. Enable it with `set warble on` for
training or attention-tone use.

### Finish Beacon (MO6)

Some ARDF events include a sixth continuous beacon at the finish line, sometimes
called `MO6`. It transmits on a **separate frequency** so hunters can home in on
the finish without interfering with the five foxes. This firmware supports a
continuous beacon mode:

```text
set mode beacon
set fox MO6
set beacon_id 60
```

In beacon mode, the firmware keys PTT continuously and re-sends the CW ID every
`beacon_id` seconds. Use a different radio frequency for the finish beacon. Set
`set mode fox` to return to normal scheduled fox operation.

### Bands, Polarization, And Power

IARU ARDF events use two main bands. The firmware does not enforce these — the
attached radio determines the actual frequency and power — but the operator is
responsible for compliance.

| Band | Frequency range | Polarization | Typical power |
| --- | --- | --- | --- |
| 2 m (VHF) | 144.500 - 144.800 MHz | Horizontal | 1 - 5 W |
| 80 m (HF) | 3.500 - 3.600 MHz | Vertical | 1 - 5 W |

Key points:

- **2 m foxes** use horizontal polarization. A horizontal dipole or Yagi is
  standard. Hunters use horizontal antennas on their receivers.
- **80 m foxes** use vertical polarization. A vertical whip or loop antenna is
  standard.
- **Power** is typically 1 to 5 watts. Higher power is rarely needed and may
  violate event rules or local regulations.
- Always confirm the exact frequency with your local band plan and event
  organizer before transmitting.
- Follow your local amateur radio regulations for identification, power limits,
  and permitted frequencies.

## Warble Tone

A warble is an audio tone that alternates between two frequencies, for example:

```text
700 Hz -> 900 Hz -> 700 Hz -> 900 Hz
```

It is easier to notice than silence or a steady carrier. In this firmware, the
warble starts after the callsign and fox identifier are sent. The warble is
**disabled by default** for IARU standard compliance — competition foxes send a
CW ID followed by a steady carrier. Enable it with `set warble on` for training
or attention-tone use.

## Hardware Roles

The ESP32 controls low-level timing and audio:

| Part | Role |
| --- | --- |
| ESP32 | Timing, Morse, warble tone, PTT control, serial config, web admin UI |
| Radio/transmitter | Generates the actual RF signal |
| PTT interface | Lets the ESP32 key the radio safely |
| Audio filter/level control | Feeds clean audio into the radio mic input |
| Battery divider | Optional voltage measurement for low-battery cutoff |
| OLED/TFT/E-Ink display | Live status display + on-screen settings menu on supported boards |

Use a transistor, MOSFET, relay, or optocoupler for PTT. Do not connect an ESP32
GPIO directly to an unknown radio PTT line until the radio voltage and current
are confirmed.

## Configuration Methods

The beacon supports three configuration methods:

1. **Compile-time**: Edit `include/beacon_config.h` before building.
2. **Serial Monitor**: USB commands at 115200 baud after flashing.
3. **Web Admin UI**: WiFi AP with captive portal — connect from a phone or
   laptop to `FoxBeacon-XXXX` and browse to `http://192.168.4.1/`. The AP
   auto-offs after a configurable timeout (default 10 min) of no activity to
   save power.

All three control the same settings. The web UI is the easiest for field use
since no cable is needed after the first upload.

Boards with a display also have an **on-screen settings menu** (double-click the
button) for quick on/off toggles: WiFi AP, warble, fox sync, battery monitor,
beacon/fox mode, and display eco mode. This is a convenience subset — full
configuration (callsign, fox ID, timing, CW, PTT, etc.) requires the web UI or
serial monitor.

## Legal And Event Notes

Set the callsign to the licensed station or club callsign required by your local
rules. Keep transmit power low enough for the event area, use suitable antennas
or dummy loads during testing, and confirm the event frequency is allowed before
field use.
