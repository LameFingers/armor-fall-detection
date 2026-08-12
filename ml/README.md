# Armor Machine Learning

This folder will document the data and model-development workflow used for
Armor's on-device fall detection.

## Planned workflow

1. Collect labeled BMI270 accelerometer and gyroscope data.
2. Record normal activities and controlled fall-like events.
3. Clean, segment, and label the sensor data.
4. Establish a conventional threshold-based baseline detector.
5. Train and evaluate TinyML models using Edge Impulse.
6. Compare models using fall detection, false alarms, latency, memory use, and
   expected power consumption.
7. Export the selected model for deployment on the ESP32-S3.

## Potential input features

- Three-axis acceleration
- Three-axis gyroscope data
- Acceleration magnitude
- Gyroscope magnitude
- Jerk or rapid change in acceleration
- Orientation change
- Short-window motion energy
- Post-event inactivity

## Data privacy

Raw data will not be published if it contains identifying or sensitive
information. The repository may include small, anonymized sample datasets or
data-format examples for reproducibility.

## Current status

Dataset collection and model training have not started.
