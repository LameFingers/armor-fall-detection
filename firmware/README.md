# Armor Firmware

This folder will contain the embedded firmware that runs on both Armor boards.

## Boards

Both firmware targets run on the **LilyGO LoRa32 915 MHz** development board
(also sold as TTGO LoRa32 / TTGO Paxcounter). Key on-board hardware used by
Armor:

| Feature | Detail |
|---------|--------|
| SoC | ESP32 (dual-core Xtensa LX6, 240 MHz) |
| LoRa radio | SX1276 / SX1278 @ 915 MHz |
| Display | 0.96-inch I²C OLED |
| Storage | SD-card slot (not used in v1) |
| Connectivity | BLE + Wi-Fi (not used in v1) |
| Battery | 3.7 V LiPo via onboard charging IC |

The external IMU is the **HiLetgo BMI160** (six-axis: 3-axis accelerometer +
3-axis gyroscope) connected over I²C.

## Firmware targets

### 1. Wearable (`firmware/wearable/`)

Planned responsibilities:

- Initialize the BMI160 IMU and configure it for 50 Hz sampling
- Collect accelerometer and gyroscope data at a consistent rate
- Prepare fixed-length motion windows for fall-detection logic
- Run a baseline threshold detector on each window
- Optionally run a deployed TinyML model as a second-stage classifier
- Manage the device state machine: idle → possible-fall → alerting →
  cancelled / confirmed
- Drive the buzzer and status LED during the alert window
- Read the cancel button to abort a false alarm
- Transmit a compact LoRa emergency packet when a fall is confirmed
- Receive an acknowledgement packet from the base station

### 2. Base station (`firmware/base-station/`)

Planned responsibilities:

- Listen for incoming LoRa packets
- Validate the packet format and device ID
- Activate the buzzer and red LED when an alert is received
- Wait for the operator acknowledgement button press
- Transmit a LoRa acknowledgement packet back to the wearable
- Display alert status on the onboard OLED

## Planned development environment

- IDE: Arduino IDE or PlatformIO
- Board package: ESP32 Arduino core (espressif/arduino-esp32)
- Key libraries (to be confirmed against actual board variant):
  - `BMI160-Arduino` — IMU driver
  - `RadioLib` or `arduino-LoRa` — SX1276/SX1278 LoRa driver
  - `Adafruit SSD1306` + `Adafruit GFX` — OLED display
- ML deployment (stretch goal): Edge Impulse Arduino library or equivalent
  exported C++ model

## Current status

Firmware development has not started. Parts have been ordered. Pin assignments
and library selection will be confirmed once the boards are in hand.
