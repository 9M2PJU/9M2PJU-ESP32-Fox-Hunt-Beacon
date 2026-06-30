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
5. Sends an optional warble tone until the transmit window ends.
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

## Warble Tone

A warble is an audio tone that alternates between two frequencies, for example:

```text
700 Hz -> 900 Hz -> 700 Hz -> 900 Hz
```

It is easier to notice than silence or a steady carrier. In this firmware, the
warble starts after the callsign and fox identifier are sent. Disable it if the
event should use CW ID only.

## Hardware Roles

The ESP32 controls low-level timing and audio:

| Part | Role |
| --- | --- |
| ESP32 | Timing, Morse, warble tone, PTT control, serial configuration |
| Radio/transmitter | Generates the actual RF signal |
| PTT interface | Lets the ESP32 key the radio safely |
| Audio filter/level control | Feeds clean audio into the radio mic input |
| Battery divider | Optional voltage measurement for low-battery cutoff |

Use a transistor, MOSFET, relay, or optocoupler for PTT. Do not connect an ESP32
GPIO directly to an unknown radio PTT line until the radio voltage and current
are confirmed.

## Legal And Event Notes

Set the callsign to the licensed station or club callsign required by your local
rules. Keep transmit power low enough for the event area, use suitable antennas
or dummy loads during testing, and confirm the event frequency is allowed before
field use.
