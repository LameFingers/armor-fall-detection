# Armor Machine Learning

This folder will document the data and model-development workflow used for
Armor's on-device fall detection.

## Planned workflow

1. Collect labeled BMI160 accelerometer and gyroscope data.
2. Record normal activities and controlled fall-like events.
3. Clean, segment, and label the sensor data.
4. Establish a conventional threshold-based baseline detector.
5. Train and evaluate TinyML models using Edge Impulse.
6. Compare models using fall detection, false alarms, latency, memory use, and
   expected power consumption.
7. Export the selected model for deployment on the ESP32-S3.

## Potential input features

- Three-axis acceleration (BMI160 accel, ±2 g to ±16 g range)
- Three-axis gyroscope data (BMI160 gyro, ±125 °/s to ±2000 °/s range)
- Acceleration magnitude
- Gyroscope magnitude
- Jerk or rapid change in acceleration
- Orientation change
- Short-window motion energy
- Post-event inactivity

## Target deployment platform

- Board: LilyGO LoRa32 915 MHz (ESP32, 240 MHz dual-core, ~320 KB RAM)
- Inference must complete well within the 20 ms sampling interval at 50 Hz
- Exported model format: Edge Impulse Arduino library (`.zip`) or raw C array

## Data privacy

Raw data will not be published if it contains identifying or sensitive
information. The repository may include small, anonymized sample datasets or
data-format examples for reproducibility.

## Current status

Dataset collection and model training have not started. Parts have been ordered.
