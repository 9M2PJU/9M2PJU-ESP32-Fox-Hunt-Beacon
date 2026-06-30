# Field Checklist

Use this checklist before hiding a beacon for an amateur radio fox hunt.

## Bench Test

- Build the firmware for the exact ESP32 board environment.
- Set the callsign, fox ID, transmit time, idle time, and startup delay.
- Run `show` in Serial Monitor and confirm the saved configuration.
- Run `ptt_test` with no radio connected and confirm the PTT output releases.
- Run `test` into a dummy load or very low-power setup.
- Listen on a second receiver and confirm the CW ID is readable.
- Reduce audio level if the received tone sounds distorted or too wide.
- Confirm the radio returns to receive after each transmission.

## Radio Setup

- Program the radio frequency, tone/squelch settings, bandwidth, and power.
- Lock the radio keypad if the model supports it.
- Use the lowest power level that fits the event area.
- Use a known-good antenna or dummy load for testing.
- Confirm the event frequency and ID requirements are allowed by local rules.

## Power Setup

- Fully charge the radio battery and ESP32 battery or power bank.
- Confirm regulators do not overheat during a full transmit cycle.
- If battery monitoring is enabled, confirm the ADC reading with a multimeter.
- Carry a simple way to power off the beacon quickly.

## Field Deployment

- Use a startup delay long enough to hide the beacon safely.
- Keep audio/PTT cables strain-relieved inside the enclosure.
- Keep the ESP32 and audio wiring away from the radio antenna feed.
- Weatherproof the enclosure if rain or wet grass is possible.
- Label the beacon with owner contact details.
- Record the hidden location so the beacon can be recovered.

## Recovery

- Power off the beacon before disconnecting radio cables.
- Check the radio and ESP32 batteries after the hunt.
- Inspect cables and connectors for damage.
- Save any timing or audio-level lessons back into `include/beacon_config.h`.
