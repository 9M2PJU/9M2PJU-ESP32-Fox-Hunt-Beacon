# Field Checklist

Use this checklist before hiding a beacon for an amateur radio fox hunt.

## Bench Test

- Build the firmware for the exact ESP32 board environment.
- Set the callsign, fox ID, transmit time, idle time, and startup delay.
- For a multi-fox event, confirm `fox_sync on` and verify the auto startup
  delay shown by `show` matches the fox slot (MOE=0 s, MOI=60 s, etc.).
- For a finish beacon, set `mode beacon` and confirm the beacon ID interval.
- Run `show` in Serial Monitor and confirm the saved configuration.
- Connect to the `FoxBeacon-XXXX` WiFi AP from a phone and verify the web UI
  loads at `http://192.168.4.1/`. Confirm settings are editable and save.
- Verify the AP auto-off timeout is configured (default 10 min). Disconnect from
  the AP and wait for the timeout — confirm the AP disappears. Turn it back on
  via the on-screen menu or `set wifi_ap on`.
- If the board has a display, confirm the startup screen shows "CALLSIGN Fox"
  and the firmware version for 3 seconds, then transitions to the status screen.
- Verify the status screen shows callsign, fox ID, mode, state, timing, and
  AP IP (if WiFi AP is on).
- Double-click the button to open the settings menu. Verify you can navigate
  (single click) and toggle items (double click). Toggle WiFi AP off and back
  on to confirm the AP restarts. Long-hold or wait 30s to exit the menu.
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
