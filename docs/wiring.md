# Wiring ESP32 To A Cheap Walkie Talkie

This firmware is designed for cheap handheld radios that expose microphone,
ground, and push-to-talk wiring through an accessory connector. Many low-cost
radios use a Kenwood-style two-pin connector, but pinouts still vary. Confirm
the radio pinout before wiring.

## Recommended Interface

Use isolation or transistor switching. Do not connect an ESP32 GPIO directly to
a radio PTT line until you know the radio voltage and current.

```text
ESP32 GPIO 25 ---- resistor ---- NPN base
NPN emitter -------------------- radio/accessory ground
NPN collector ------------------ radio PTT line

ESP32 GPIO 26 ---- RC filter ---- coupling capacitor ---- level pot ---- radio mic input
ESP32 GND ----------------------------------------------- radio/accessory ground
```

For better protection, replace the NPN PTT switch with an optocoupler. This is
especially useful if the radio accessory ground is noisy or if the radio is
powered from a different battery.

## Audio Level

Start with the ESP32 audio level very low. Handheld microphone inputs are
sensitive and will over-deviate easily.

Suggested starting audio chain:

```text
GPIO 26 -> 1 k resistor -> 100 nF capacitor to ground -> 1 uF coupling capacitor -> 10 k trimpot -> mic input
```

Set the radio volume low while testing. Listen on a second receiver and adjust
the trimpot until the CW and tone are clean but not distorted.

## PTT Notes

Most simple handhelds transmit when the radio PTT line is shorted to radio
ground. With the recommended NPN transistor wiring above, the ESP32 GPIO goes
HIGH, the transistor turns on, and the radio PTT line is pulled to ground. That
means the firmware should use `active_high`, which is the default.

Use this command for the recommended NPN/MOSFET/opto driver:

```text
set ptt active_high
```

Use `active_low` only if your interface circuit keys the radio when the ESP32
GPIO is LOW.

Before connecting a radio, run:

```text
ptt_test
```

Check the PTT output with an LED or multimeter. It should turn on for about 1.2
seconds, then turn off.

## Default Firmware Pins

| Function | ESP32 GPIO |
| --- | ---: |
| PTT control | 25 |
| Audio tone | 26 |
| Status LED | 2 |
| Test button | 0 |
| Battery ADC | 34 |

ESP32-S3 and ESP32-C3 environments override some pins in `platformio.ini`
because those boards expose different ADC and boot-safe pins.

## First Test

1. Upload the firmware.
2. Connect only an LED or multimeter to the PTT output first.
3. Open the serial monitor at 115200 baud.
4. Type `ptt_test`.
5. Confirm that PTT switching works and releases reliably.
6. Connect the PTT interface to the radio.
7. Type `test` and confirm that audio output works.
8. Test into a dummy load or at the lowest practical power.
9. Connect audio only after the PTT switching is reliable.

## Cheap Radio Caveats

- Some radios combine mic bias, PTT, and audio in ways that need extra
  resistors.
- Some speaker-mic cables use non-obvious wire colors.
- Some radios produce RF noise that can reset a nearby ESP32. Keep wires short,
  add decoupling, and use a separate enclosure ground plan if needed.
- VOX can work for tests, but wired PTT is more predictable for a beacon.
