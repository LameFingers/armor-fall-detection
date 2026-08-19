# Armor

## Edge-AI Fall Detection and Long-Range Emergency Alerting for Remote Workers

Armor is a wearable safety system being developed for farmers and other people
who work alone in remote areas. It aims to detect potential falls locally using
motion sensors and TinyML, then send an emergency alert through LoRa when the
wearer cannot respond.

## The problem

A fall or serious injury can leave a remote worker unable to call for help.
Existing fall-detection devices can produce false alarms, miss genuine falls,
depend on smartphone or internet access, and raise privacy concerns by sending
personal motion data to the cloud.

## Proposed solution

Armor will use a BMI160 motion sensor and a LilyGO LoRa32 (TTGO) development
board to analyze motion data directly on the wearable device. When a possible
fall is detected, Armor will alert the wearer with a buzzer and LED and allow
time to cancel a false alarm. If there is no response, the device will send an
emergency message over LoRa to a second LilyGO LoRa32 base station.

## Planned system components

- LilyGO LoRa32 915 MHz development board (ESP32, SX1276/SX1278, OLED, SD, BLE, Wi-Fi)
- HiLetgo BMI160 six-axis IMU (accelerometer and gyroscope)
- TinyML model trained with labeled motion data
- Buzzer and LED for local wearer alerts
- Second LilyGO LoRa32 board acting as base station / receiver
- Rechargeable 3.7 V LiPo battery and 3D-printed enclosure
- Python-based data collection, processing, and testing tools

## Current Working Prototype

A physically tested breadboard prototype is now operational.  Both sender and
receiver boards have been assembled and the v1 firmware has been verified
end-to-end.

> This is a **working baseline prototype** requiring controlled testing and
> further validation before any safety-critical use.  It is not a validated
> medical fall detector and is not safety-certified in any way.

### What the prototype does

- The **sender** (LILYGO T3 V1.6.1 + DFRobot Gravity BMI160) reads
  accelerometer and gyroscope values from the BMI160 at 500 ms intervals.
- The sender displays live acceleration magnitude on its built-in OLED and
  shows normal monitoring status through a green RGB LED.
- The sender performs a **baseline fast-motion threshold test**: when total
  acceleration magnitude exceeds `MOTION_THRESHOLD_G` (1.2 g default) and the
  alert cooldown has elapsed, an alert fires.
- On alert the sender LED turns red, the KY-006 passive buzzer sounds, and a
  `FALL_ALERT` LoRa packet is broadcast.
- The **receiver** (LILYGO T3 V1.6.1) listens continuously for `FALL_ALERT`
  packets.
- On receipt the receiver shows the alert, RSSI, and SNR on its OLED, turns
  its LED red, and sounds its KY-006 buzzer.
- Both boards return to their normal green/listening state after approximately
  two seconds.

### Firmware

| Board    | Sketch |
|----------|--------|
| Sender   | [`firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino`](firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino) |
| Receiver | [`firmware/receiver_lora_alert/receiver_lora_alert.ino`](firmware/receiver_lora_alert/receiver_lora_alert.ino) |

### Key documentation

| Document | Description |
|----------|-------------|
| [`docs/hardware-wiring.md`](docs/hardware-wiring.md) | GPIO pin assignments and breadboard wiring for both boards |
| [`docs/firmware-setup.md`](docs/firmware-setup.md) | Arduino IDE setup, library installation, and upload instructions |
| [`docs/baseline-motion-detection.md`](docs/baseline-motion-detection.md) | How the threshold detector works, limitations, and future improvements |
| [`docs/testing-log-template.md`](docs/testing-log-template.md) | Template for recording physical test sessions |
| [`hardware/README.md`](hardware/README.md) | Bill of materials for the v1 prototype |
| [`evidence/`](evidence/) | Wiring photos, OLED screenshots, Serial Monitor captures, and test data |

---

## Project status

**Working baseline prototype.** Both boards have been assembled on breadboards
and the v1 firmware has been physically tested end-to-end.  Threshold
calibration, controlled testing, and data collection for TinyML development
are the next steps.

## IBM Bob usage

IBM Bob is being used as the primary AI development partner for system planning,
architecture, code development, test design, debugging, and project
documentation. Development evidence will be maintained in
[`docs/bob-development-log.md`](docs/bob-development-log.md).

## Repository structure

This repository will contain embedded firmware, machine-learning workflows,
hardware documentation, test results, and IBM Bob development evidence.

## Safety note

Armor is an early-stage prototype and is not certified medical equipment or a
replacement for emergency services.
