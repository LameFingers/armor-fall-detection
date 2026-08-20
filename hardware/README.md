# ARMOR Hardware

This folder contains wiring diagrams, the bill of materials, and prototype
photographs for the ARMOR project.

See [`docs/hardware-wiring.md`](../docs/hardware-wiring.md) for full
GPIO pin assignments and breadboard wiring instructions.

---

## Bill of materials — v1 working prototype

| Qty | Component                                 | Notes                                                         |
|-----|-------------------------------------------|---------------------------------------------------------------|
| 2   | LILYGO T3 V1.6.1 LoRa board              | ESP32 + SX1276 + built-in SSD1306 OLED; one sender, one receiver |
| 1   | DFRobot Gravity BMI160 / SEN0250          | 6-axis IMU (accelerometer + gyroscope); sender only           |
| 2   | Common-anode RGB LED                      | One per board; indicator for monitoring and alert states      |
| 2   | KY-006 passive buzzer module              | One per board; sender uses differential drive, receiver uses LEDC PWM |
| 6   | 220–330 Ω resistors                       | Three per board (one per RGB LED color channel)               |
| 2   | Normally-open tactile push buttons        | One per board; GPIO 2 → button → GND; sender = cancel, receiver = acknowledge |
| 1   | PKCELL LP503562 LiPo battery (3.7 V)     | Sender only; connects to T3 V1.6.1 JST battery connector      |
| —   | Breadboards                               | One per board for first prototype assembly                    |
| —   | Jumper wires                              | Male-to-male and male-to-female assortment                    |
| 2   | Matching LoRa antennas                    | Must match board frequency (915 MHz); attach before powering on |
| 2   | Micro-USB cables                          | For firmware upload and serial monitoring                     |

> **Antenna note:** Both LoRa antennas must be attached before any firmware
> is uploaded or power is applied.  Operating the SX1276 radio without an
> antenna may damage the output stage.

> **Frequency note:** 915 MHz LoRa is used in Australia, North America, and
> some other regions.  Confirm local regulations before operating.

---

## Future hardware work

The following hardware improvements are planned for later development stages:

| Area                     | Description                                                            |
|--------------------------|------------------------------------------------------------------------|
| Battery                  | 3.7 V LiPo cell and USB charging circuit for untethered operation      |
| Charging circuit         | On-board or breakout TP4056/MCP73831 charging for the LiPo cell        |
| Enclosure                | 3D-printed case for the wearable sender unit                           |
| Custom PCB               | Purpose-designed PCB to replace breadboard wiring                     |
| Field durability testing | Mechanical and environmental testing for outdoor remote worker use     |

---

## Current status

Both boards have been assembled on breadboards and the v1 firmware has been
physically tested.  Wiring photographs are stored in [`evidence/`](../evidence/).
