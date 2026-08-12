# Armor Testing and Validation Plan

## Purpose

Armor must distinguish genuine falls from normal movement while minimizing false
alarms. This document will track test procedures, measured results, failures,
and design changes.

## Planned evaluation areas

- Fall detection rate
- False-positive rate during normal daily activities
- Detection latency
- TinyML inference time and memory use on the ESP32-S3
- Battery and power-use estimates
- Local alert and user-cancel behavior
- LoRa transmission range and packet reliability
- Behavior during sensor, power, or communication faults

## Planned activities

The project will collect and label sensor data from normal movement, including
walking, sitting, bending, turning, lifting equipment, and device drops.
Controlled and safe fall-like test scenarios will be used only when appropriate.

## Current status

No hardware testing has been performed yet. This document will be updated as
the prototype and test dataset are developed.

## Safety note

Armor is an early-stage prototype. Tests must be planned to avoid injury, and
results must not be presented as medical-device certification.
