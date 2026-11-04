# ESP8266 Smart Home Automation System

A local smart home automation prototype built around an **ESP8266 NodeMCU**, a PIR motion sensor, a digital sound sensor and a relay-controlled light.

The ESP8266 creates its own Wi-Fi access point and serves a control dashboard. A phone or computer can connect directly to the ESP8266 and operate the system through a web browser. An internet connection is not required.

> **Source note:** The original `.ino` and web portal source were deleted and could not be recovered from the available project history. This repository contains a reconstruction based on the documented project behavior, not a verified byte-for-byte copy of the original files.

## Features

- Manual light ON/OFF control from the web portal.
- PIR motion detection toggles the light when a new motion event is detected.
- Sound sensor detection (such as a clap) toggles the light.
- Independent enable/disable controls for the PIR and sound sensors.
- ESP8266-hosted Wi-Fi access point and local web dashboard.
- No activity log.
- No LDR in this final project version.

## Hardware

- ESP8266 NodeMCU development board
- PIR motion sensor
- Digital sound sensor module
- 1-channel relay module
- AC light/bulb and suitable electrical enclosure
- Jumper wires and a suitable power supply

## Pin connections

The following pin map is used by the reconstructed sketch. **These are assumed pins, not confirmed original wiring.** Change the constants in `HomeAutomation.ino` if your physical circuit uses different pins.

| Component | Module pin | NodeMCU |
|---|---|---|
| PIR sensor | OUT | D5 (GPIO14) |
| Sound sensor | DO | D6 (GPIO12) |
| Relay module | IN | D1 (GPIO5) |
| Sensors | VCC | Appropriate rated supply |
| Sensors | GND | GND |
| Relay module | GND | GND |

Check the voltage requirements of each module before connecting it. Do not assume every relay board or sensor accepts 3.3 V logic.

### Relay logic

The sketch is configured for an **active-LOW relay module**. If your relay board is active-HIGH, change:

```cpp
const bool RELAY_ACTIVE_LOW = true;
```

to `false`.

## Software and libraries

- Arduino IDE
- ESP8266 board support package for Arduino
- `ESP8266WiFi` (included with the ESP8266 core)
- `ESP8266WebServer` (included with the ESP8266 core)

Select the correct ESP8266 board and port in Arduino IDE, then upload `HomeAutomation.ino`.

## Connecting to the dashboard

1. Upload the sketch to the ESP8266 NodeMCU.
2. Power the board.
3. Connect your phone or computer to the Wi-Fi network:
   - **SSID:** `HomeAutomation`
   - **Password:** `12345678`
4. Open `http://192.168.4.1` in a browser.
5. Use the dashboard to control the light and enable or disable each sensor.

The access point name and password can be changed near the top of `HomeAutomation.ino`.

## How it works

- **Manual control:** The dashboard sends a request to the ESP8266 to switch the relay ON or OFF.
- **PIR sensor:** A new LOW-to-HIGH motion signal toggles the light when the PIR sensor is enabled.
- **Sound sensor:** A new LOW-to-HIGH digital trigger toggles the light when the sound sensor is enabled. A short debounce interval reduces repeated triggers.
- **Sensor controls:** Disabling a sensor prevents it from changing the light state. The sensor remains physically connected.
- **Web portal:** The ESP8266 hosts the page and a small JSON state endpoint on its local access point.

The exact trigger polarity and sensitivity of sound sensor modules can vary. If your module outputs LOW on detection, the sound detection logic will need to be inverted.

## Repository structure

```text
home-automation/
├── HomeAutomation.ino
├── README.md
└── circuit/
    ├── home_automation_box.png
    ├── web_portal.png
    └── block_diagram.png
```

Add your three project images to the `circuit/` folder using the exact filenames above. The folder includes a `.gitkeep` placeholder so Git tracks it before the images are added.

## Safety

The relay may switch mains AC voltage. Mains wiring can cause serious injury, fire or death. Keep all mains terminals enclosed, use a correctly rated relay and insulation, and have the AC-side wiring checked and completed by a qualified person. Disconnect mains power before changing any wiring. Test the low-voltage control side separately.

## Project status

This repository documents a student-built smart home automation prototype. The source is a reconstructed implementation based on the surviving project requirements; verify the assumed pin assignments, relay polarity and sensor output behavior against the actual hardware before use.
