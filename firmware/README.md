# Armor Firmware

This folder will contain the embedded firmware that runs on the Armor wearable.

## Planned responsibilities

- Initialize and read data from the BMI270 IMU
- Sample accelerometer and gyroscope data at a consistent rate
- Prepare motion windows for fall-detection logic
- Run a baseline threshold detector and the deployed TinyML model
- Manage possible-fall, cancellation, and confirmed-alert states
- Control the buzzer, LED, and optional user-cancel button
- Send emergency packets through LoRa
- Monitor device status, including battery level when implemented

## Planned platform

- Microcontroller: ESP32-S3
- Sensor: BMI270
- Development environment: Arduino IDE or PlatformIO
- ML deployment: Edge Impulse Arduino library or equivalent exported model

## Current status

Firmware development has not started. This file will be replaced or expanded
when the first hardware prototype is assembled.
