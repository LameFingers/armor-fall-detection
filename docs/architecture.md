# Armor System Architecture

## Overview

Armor is an edge-AI wearable safety system designed to identify probable falls
and request help for remote workers. The system performs motion analysis locally
on the wearable, provides the wearer an opportunity to cancel a false alarm,
and sends an emergency notification by LoRa if the wearer does not respond.

## Planned information flow

1. The BMI270 IMU measures acceleration and rotational motion.
2. The ESP32-S3 samples and preprocesses the sensor data.
3. A baseline detector and TinyML model evaluate the motion window locally.
4. If a possible fall is detected, Armor activates a local buzzer and LED alert.
5. The wearer can cancel the alert if they are safe.
6. If no cancellation occurs within the configured time window, Armor sends an
   emergency packet through LoRa.
7. A LoRa gateway receives the packet and can display, log, or forward the alert.

## Main subsystems

- Wearable sensing: BMI270 accelerometer and gyroscope
- Edge processing: ESP32-S3 microcontroller
- AI inference: TinyML model deployed directly on the wearable
- User feedback: buzzer, LED, and optional cancel button
- Communications: LoRa transmitter and receiving gateway
- Power and enclosure: rechargeable LiPo battery, charging circuit, and
  3D-printed enclosure

## Current status

Planning phase. Hardware integration, data collection, model training, and
prototype validation are still in progress.
