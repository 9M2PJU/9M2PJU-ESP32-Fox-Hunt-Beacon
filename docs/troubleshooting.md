# Troubleshooting

Use this guide when the beacon does not build, upload, key, transmit cleanly, or
behave as expected in the field.

## Build Problems

| Symptom | Check |
| --- | --- |
| `pio: command not found` | Install PlatformIO or use `.venv/bin/pio` if this repository has a virtual environment. |
| Unknown board environment | Run `pio project config` or check the environment names in `platformio.ini`. |
| Compile errors after editing config | Check quotes, semicolons, and numeric values in `include/beacon_config.h`. |
| Wrong pins on ESP32-S3/C3 | Check `platformio.ini`; those environments override some default pins. |

## Upload Problems

| Symptom | Check |
| --- | --- |
| Board not detected | Try another USB cable, USB port, or driver. Many cables are charge-only. |
| Upload starts then times out | Hold `BOOT` while upload connects, then release it after writing starts. |
| Permission denied on Linux | Add the user to the serial device group or use the correct udev rules. |
| Upload works once, then fails | Disconnect radio/PTT wiring and retry. External wiring may hold a boot strap pin. |

## Serial Monitor Problems

| Symptom | Check |
| --- | --- |
| No text appears | Use 115200 baud and press the ESP32 reset button. |
| Garbled text | Confirm monitor speed is 115200. |
| Commands do nothing | Type one command per line and press Enter. |
| Settings seem ignored | Run `show`; saved flash settings override `include/beacon_config.h`. |
| Need to restore firmware defaults | Run `defaults`, then `show`. |

## PTT Problems

| Symptom | Likely cause | Fix |
| --- | --- | --- |
| Radio never keys | Wrong PTT polarity, wrong cable pin, no shared ground, or failed interface transistor. | Run `ptt_test`, measure GPIO and PTT line, then try `set ptt active_high` or `set ptt active_low`. |
| Radio stays keyed | PTT polarity is reversed or the interface is wired incorrectly. | Disconnect radio, test GPIO with a meter, then correct polarity or wiring. |
| ESP32 resets when radio transmits | RF feedback or power sag. | Use shorter leads, add decoupling, separate radio and ESP32 power, lower RF power, and improve enclosure grounding. |
| PTT clicks but no RF | Radio is locked, wrong channel, dead battery, or PTT line is not the correct pin. | Test the radio manually without the ESP32. |

Never debug unknown PTT wiring at high RF power. Use a dummy load or the lowest
practical power.

## Audio Problems

| Symptom | Likely cause | Fix |
| --- | --- | --- |
| No tone is heard | Audio pin not wired, trimpot too low, wrong mic pin, or radio muted by tone squelch. | Run `test`, check GPIO 26/audio pin path, open squelch on the receiver. |
| Audio is distorted | Mic input is overdriven. | Turn the trimpot down and retest. |
| Signal sounds too wide | Audio level is too high for FM deviation. | Reduce audio level before changing firmware tones. |
| Morse is unreadable | CW speed too fast, tone too high/low, or received signal weak. | Try `set wpm 10` and `set tone 700`. |
| Warble is annoying or too fast | Step time is too short. | Try `set warble_step 600` or `set warble off`. |

## Beacon Timing Problems

| Symptom | Check |
| --- | --- |
| Beacon waits too long after boot | Check `set startup <seconds>`. |
| Transmit window too short | Check `set tx <seconds>`. |
| Long silence between transmissions | Check `set idle <seconds>`. |
| Test button transmits unexpectedly | GPIO 0 may be pulled low or the button is held. Check button wiring. |

Use `show` to confirm the active saved values.

## Battery Problems

| Symptom | Check |
| --- | --- |
| Beacon says low battery immediately | Battery monitor is enabled without a calibrated divider. Run `set battery off` until hardware is ready. |
| Voltage reading is wrong | Adjust `battery_scale` after comparing with a multimeter. |
| ESP32 browns out during TX | Radio current draw is pulling down the supply. Use separate supplies or a larger regulator. |
| Battery cutoff does not happen | Confirm the divider is connected to the configured ADC pin and `set battery on` was saved. |

## RF And Range Problems

| Symptom | Check |
| --- | --- |
| Very short range | Radio power low, bad antenna, wrong frequency, or poor battery. |
| Too much range | Lower radio power or use a smaller/event-approved antenna. |
| Hunters hear carrier but no ID | Audio path failed or level is too low. |
| Hunters hear ID but cannot tell fox number | Confirm `set fox MOE/MOI/MOS/MOH/MO5` and listen to the CW. |

## Field Recovery Checklist

Before leaving the site:

- Confirm every hidden beacon location is recorded.
- Carry a way to power off each beacon quickly.
- Check that every unit identifies correctly before hiding it.
- Label each enclosure with owner contact details.
- Inspect cables after recovery and save any lessons into the configuration.
