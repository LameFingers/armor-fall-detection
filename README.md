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

## Project status

**Planning and software setup.** Hardware selection and initial repository
structure are in progress.

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
