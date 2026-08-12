# Armor System Architecture

## Overview

Armor is an edge-AI wearable safety system designed to identify probable falls
and request help for remote workers. The system performs motion analysis locally
on the wearable, provides the wearer an opportunity to cancel a false alarm,
and sends an emergency notification by LoRa if the wearer does not respond.

Both the wearable and the base station run on LilyGO LoRa32 915 MHz boards
(also marketed as TTGO LoRa32). These boards combine an ESP32 SoC, an
SX1276/SX1278 LoRa radio, a 0.96-inch OLED display, an SD-card slot, and
onboard BLE and Wi-Fi (Wi-Fi and BLE are not used in version 1). The wearable
board reads the HiLetgo BMI160 IMU over I²C. The two boards communicate
directly over 915 MHz LoRa — there is no commercial LoRaWAN gateway.

## Planned information flow

1. The BMI160 IMU measures acceleration and rotational motion.
2. The wearable LilyGO LoRa32 samples and preprocesses the sensor data.
3. A baseline threshold detector (and optionally a TinyML model) evaluates the
   motion window locally on the ESP32.
4. If a possible fall is detected, Armor activates a local buzzer and LED alert.
5. The wearer can press the cancel button within the configured window if safe.
6. If no cancellation occurs within the configured time window, Armor sends a
   compact emergency packet through the onboard SX1276/SX1278 LoRa radio.
7. The second LilyGO LoRa32 (base station) receives the packet, activates its
   buzzer and red LED, and waits for an operator acknowledgement.

## Main subsystems

- Wearable sensing: HiLetgo BMI160 six-axis IMU (accelerometer + gyroscope)
- Edge processing: LilyGO LoRa32 (ESP32) — wearable unit
- AI inference: threshold-based detector; TinyML model as stretch goal
- User feedback: buzzer, status LED, and cancel button on the wearable
- Communications: direct 915 MHz LoRa link (SX1276/SX1278) between the two boards
- Base station: second LilyGO LoRa32 with buzzer, red LED, and ack button
- Power and enclosure: rechargeable 3.7 V LiPo battery, charging circuit, and
  3D-printed enclosure

## Current status

Planning phase. Hardware has been selected and ordered. Hardware integration,
data collection, model training, and prototype validation have not yet started.
