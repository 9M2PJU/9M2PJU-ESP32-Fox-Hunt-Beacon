# Installation And Upload

This guide gets the firmware onto an ESP32 board using PlatformIO.

## Requirements

- ESP32 development board.
- USB cable that supports data, not charge-only.
- Computer with Python and PlatformIO available.
- This repository cloned locally.
- Optional radio/PTT/audio hardware for later bench testing.

Do the first upload with no radio connected. Verify the ESP32 and PTT output
before wiring a transmitter.

## Install PlatformIO

Use one of these options:

- Install the PlatformIO extension in VS Code.
- Install PlatformIO Core with Python:

```sh
python -m pip install --user platformio
```

If this repository already has a local virtual environment, use:

```sh
.venv/bin/pio --version
```

Otherwise use:

```sh
pio --version
```

## Using Visual Studio Code

This is the easiest workflow for most users.

### 1. Install VS Code And PlatformIO

1. Install Visual Studio Code.
2. Open VS Code.
3. Open the Extensions view.
4. Search for `PlatformIO IDE`.
5. Install the `PlatformIO IDE` extension.
6. Restart VS Code if prompted.

After installation, VS Code should show the PlatformIO alien-head icon in the
left activity bar.

### 2. Open This Firmware Project

1. In VS Code, choose `File > Open Folder`.
2. Select the `9M2PJU-ESP32-Fox-Hunt-Beacon` repository folder.
3. Wait for PlatformIO to load the project.
4. Confirm that `platformio.ini` appears in the file explorer.

Open the folder itself, not only `src/main.cpp`. PlatformIO needs the whole
project folder so it can find `platformio.ini`, `include/`, and `src/`.

### 3. Choose Or Confirm The Board

Open `platformio.ini`. The default build target is:

```ini
[platformio]
default_envs = esp32dev
```

Use `esp32dev` for many common ESP32 DevKit boards. If you have a different
listed board, change `default_envs` to one of these:

```text
esp32dev
esp32doit-devkit-v1
lolin32
esp32-s3-devkitc-1
esp32-c3-devkitm-1
heltec-wifi-kit-32
heltec-wifi-kit-32-v2
heltec-wifi-kit-32-v3
heltec-wifi-lora-32
heltec-wifi-lora-32-v2
heltec-wifi-lora-32-v3
heltec-wireless-stick
heltec-wireless-stick-lite
heltec-wireless-stick-lite-v3
heltec-wireless-tracker
heltec-vision-master-t190
heltec-wireless-paper
heltec-vision-master-e213
heltec-vision-master-e290
heltec-capsule-sensor-v3
lilygo-t-display
lilygo-t-display-s3
lilygo-t3-s3
ttgo-lora32-v1
ttgo-lora32-v2
ttgo-lora32-v21
ttgo-t-beam
ttgo-t-oi-plus
ttgo-t-watch
ttgo-t1
ttgo-t7-v13-mini32
ttgo-t7-v14-mini32
```

### 4. Configure The Beacon

Open `include/beacon_config.h` and check these values before building:

```c
#define DEFAULT_CALLSIGN "9M2PJU"
#define DEFAULT_FOX_ID "MOE"
#define DEFAULT_STARTUP_DELAY_SECONDS 10
#define DEFAULT_TRANSMIT_SECONDS 60
#define DEFAULT_IDLE_SECONDS 240
```

Set `DEFAULT_CALLSIGN` to the licensed station callsign. Set
`DEFAULT_FOX_ID` to the ARDF fox identifier, such as `MOE` for Fox 1 or `MOI`
for Fox 2.

### 5. Build In VS Code

Use either method:

- Click the PlatformIO checkmark icon in the VS Code status bar.
- Open the PlatformIO sidebar, then choose `Project Tasks > <environment> >
  General > Build`.

The build should finish with `SUCCESS`.

### 6. Upload Firmware To The ESP32

1. Connect the ESP32 board with a data-capable USB cable.
2. Keep the radio disconnected for the first upload.
3. Click the PlatformIO right-arrow upload icon in the status bar.
4. Or use `Project Tasks > <environment> > General > Upload`.

If upload fails, hold the ESP32 `BOOT` button while upload starts, then release
it once writing begins.

### 7. Open Serial Monitor In VS Code

Use either method:

- Click the PlatformIO plug/monitor icon in the status bar.
- Or use `Project Tasks > <environment> > Platform > Monitor`.

The monitor speed is configured as 115200 baud in `platformio.ini`. Press the
ESP32 reset button and type:

```text
show
ptt_test
```

`show` prints the saved beacon configuration. `ptt_test` keys only the PTT
output briefly with no audio, which is the safest first hardware test.

### 8. Change Settings After Upload

You can change settings in two ways without rebuilding:

**Serial Monitor** (USB cable required):

```text
set call 9M2PJU
set fox MOE
set tx 60
set idle 240
set fox_sync on
show
```

**Web Admin UI** (no cable needed after first upload):

After boot, look for a WiFi network named `FoxBeacon-XXXX` on your phone or
laptop. Connect to it and browse to `http://192.168.4.1/` if the captive portal
does not auto-open. The web UI has forms for all settings and buttons for test,
PTT test, defaults, and reboot.

**On-Screen Menu** (display boards only):

Double-click the button on the status screen to open a quick settings menu with
on/off toggles for WiFi AP, warble, fox sync, battery, mode, and display eco
mode. Single-click to navigate, double-click to toggle, long-hold to exit. Full
configuration still requires the web UI or serial monitor.

Settings are saved in ESP32 flash. To restore the values from
`include/beacon_config.h`, type `defaults` in Serial Monitor or click the
Defaults button in the web UI.

## Choose A Board Environment

The default environment is `esp32dev`, which works for many ESP32 DevKit boards.

Available environments:

| Environment | Board |
| --- | --- |
| `esp32dev` | Generic ESP32 Dev Module |
| `esp32doit-devkit-v1` | DOIT ESP32 DevKit v1 |
| `lolin32` | WEMOS LOLIN32 |
| `esp32-s3-devkitc-1` | ESP32-S3-DevKitC-1 |
| `esp32-c3-devkitm-1` | ESP32-C3-DevKitM-1 |
| `heltec-wifi-kit-32` | Heltec WiFi Kit 32 |
| `heltec-wifi-kit-32-v2` | Heltec WiFi Kit 32 V2 |
| `heltec-wifi-kit-32-v3` | Heltec WiFi Kit 32 V3 |
| `heltec-wifi-lora-32` | Heltec WiFi LoRa 32 |
| `heltec-wifi-lora-32-v2` | Heltec WiFi LoRa 32 V2 |
| `heltec-wifi-lora-32-v3` | Heltec WiFi LoRa 32 V3 |
| `heltec-wireless-stick` | Heltec Wireless Stick |
| `heltec-wireless-stick-lite` | Heltec Wireless Stick Lite |
| `heltec-wireless-stick-lite-v3` | Heltec Wireless Stick Lite V3 compatibility build |
| `heltec-wireless-tracker` | Heltec Wireless Tracker (TFT, compatibility build) |
| `heltec-vision-master-t190` | Heltec Vision Master T190 (TFT, compatibility build) |
| `heltec-wireless-paper` | Heltec Wireless Paper (E-Ink, compatibility build) |
| `heltec-vision-master-e213` | Heltec Vision Master E213 (E-Ink, compatibility build) |
| `heltec-vision-master-e290` | Heltec Vision Master E290 (E-Ink, compatibility build) |
| `heltec-capsule-sensor-v3` | Heltec Capsule Sensor V3 (no display, compatibility build) |
| `lilygo-t-display` | LilyGO T-Display |
| `lilygo-t-display-s3` | LilyGO T-Display S3 |
| `lilygo-t3-s3` | LilyGO T3-S3 |
| `ttgo-lora32-v1` | TTGO LoRa32 OLED V1 |
| `ttgo-lora32-v2` | TTGO LoRa32 OLED V2 |
| `ttgo-lora32-v21` | TTGO LoRa32 OLED v2.1.6 |
| `ttgo-t-beam` | TTGO T-Beam |
| `ttgo-t-oi-plus` | TTGO T-OI Plus ESP32-C3 |
| `ttgo-t-watch` | TTGO T-Watch |
| `ttgo-t1` | TTGO T1 |
| `ttgo-t7-v13-mini32` | TTGO T7 V1.3 Mini32 |
| `ttgo-t7-v14-mini32` | TTGO T7 V1.4 Mini32 |

If your board is not listed, start with `esp32dev` for classic ESP32 boards, or
add a new PlatformIO environment in `platformio.ini`.

For Heltec-specific notes, see [heltec-boards.md](heltec-boards.md). For
LilyGO and TTGO notes, see [lilygo-boards.md](lilygo-boards.md).

## Configure Before Upload

Edit `include/beacon_config.h` before building if you want the firmware image to
contain your default callsign, fox ID, timing, pins, and battery settings.

Minimum values to check:

```c
#define DEFAULT_CALLSIGN "9M2PJU"
#define DEFAULT_FOX_ID "MOE"
#define DEFAULT_TRANSMIT_SECONDS 60
#define DEFAULT_IDLE_SECONDS 240
```

See [configuration.md](configuration.md) for the full setup guide.

## Build

Build the default environment:

```sh
pio run
```

Build a specific environment:

```sh
pio run -e esp32doit-devkit-v1
```

## Upload

Connect the ESP32 by USB, then upload:

```sh
pio run -t upload
```

Upload a specific environment:

```sh
pio run -e esp32doit-devkit-v1 -t upload
```

If upload fails, hold the board `BOOT` button while PlatformIO starts
connecting, then release it after the upload begins. Some boards need this only
occasionally.

## Open Serial Monitor

Open the serial monitor at 115200 baud:

```sh
pio device monitor -b 115200
```

Press reset on the board. You should see the beacon banner and configuration.
Type:

```text
show
```

Then test without a radio connected:

```text
ptt_test
```

## First Bench Test Order

1. Upload firmware with no radio connected.
2. Run `show` and confirm callsign, fox ID, timing, and pins.
3. Run `ptt_test` and check GPIO 25 with an LED or multimeter.
4. Wire the PTT interface only.
5. Run `ptt_test` again and confirm the radio keys and releases.
6. Add audio wiring with the level trimpot turned low.
7. Run `test` and listen on a second receiver.
8. Increase audio level slowly until CW is readable and not distorted.

Use a dummy load or the lowest practical RF power while testing.
