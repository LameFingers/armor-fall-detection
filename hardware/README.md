# Armor Hardware

This folder will contain wiring diagrams, the bill of materials, enclosure
files, and prototype photographs.

## Bill of materials (v1 prototype)

| Qty | Component | Notes |
|-----|-----------|-------|
| 2 | LilyGO LoRa32 915 MHz (TTGO LoRa32 / TTGO Paxcounter) | ESP32 + SX1276/SX1278 + OLED; one wearable, one base station |
| 1 | HiLetgo BMI160 6-axis IMU breakout | I²C; wearable unit only |
| 1 | Active buzzer 3.3 V (wearable) | Alert during cancel window |
| 1 | Active buzzer 3.3 V (base station) | Alert on received fall packet |
| 1 | Red LED + current-limiting resistor (base station) | Visual alert |
| 1 | Status LED + current-limiting resistor (wearable) | Indicates alert state |
| 1 | Tactile push button (wearable) | Cancel false alarm |
| 1 | Tactile push button (base station) | Operator acknowledgement |
| 1 | 3.7 V LiPo battery (wearable) | Capacity TBD |
| — | Breadboard and jumper wires | First prototype |
| — | Protoboard / perfboard | More durable prototype |
| — | 3D-printed enclosure | Final prototype |

> **Frequency note:** 915 MHz LoRa is used in Australia, North America, and
> some other regions. Confirm local regulations before operating.

## Development sequence

1. Board bring-up: verify OLED and serial output on both LoRa32 boards
2. BMI160 bring-up: read accelerometer and gyroscope over I²C on the wearable board
3. Local alert: test buzzer, LEDs, and cancel button
4. LoRa link: transmit a test packet between the two boards and verify reception
5. Fall-detection logic: implement threshold detector with BMI160 data
6. End-to-end demo: fall → alert → no cancel → LoRa TX → base station buzzer/LED
7. Battery-powered test
8. Transfer to protoboard and 3D-printed enclosure

## Current status

Hardware has been ordered but not yet assembled. Wiring diagrams will be added
once the boards are in hand and pin assignments are confirmed.
