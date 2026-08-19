# ARMOR Machine Learning

This folder is reserved for the ARMOR machine-learning development workflow.
No trained models or placeholder model files exist yet — this folder will be
populated as the project progresses through data collection and model
development.

---

## Planned workflow

This folder will contain work across the following stages:

| Stage                    | Description                                                              |
|--------------------------|--------------------------------------------------------------------------|
| Data organization        | References and scripts for managing raw and processed sensor data        |
| Data cleaning            | Scripts to remove corrupt samples, fill gaps, and align timestamps       |
| Feature engineering      | Code to compute windowed features from raw BMI160 data                   |
| Model training           | Training scripts or Edge Impulse project references                      |
| Model evaluation         | Evaluation notebooks, confusion matrices, and metric summaries           |
| TinyML deployment        | Instructions and scripts for deploying an exported model to the ESP32    |

---

## Data to be collected

Labeled IMU data will be recorded from the BMI160 on the sender board during
controlled test sessions and will be stored in [`evidence/`](../evidence/).

Target labels:
- Normal activities (walking, standing, sitting, tool use, arm movements)
- Controlled fall-like events (simulated drops and sharp movements on soft surfaces)

---

## Candidate input features

- Three-axis accelerometer (BMI160, ±2 g to ±16 g)
- Three-axis gyroscope (BMI160, ±125 °/s to ±2000 °/s)
- Acceleration magnitude
- Gyroscope magnitude
- Jerk (rate of change of acceleration)
- Orientation change
- Short-window motion energy
- Post-event inactivity window

---

## Target deployment

- **Board:** LILYGO T3 V1.6.1 (ESP32, 240 MHz dual-core, ~320 KB RAM)
- **Inference requirement:** must complete within the sensor sampling interval
- **Exported model format:** Edge Impulse Arduino library (`.zip`) or raw
  C array for direct inclusion in the `.ino` sketch

---

## Current status

Dataset collection and model training have not started.  The current prototype
uses the baseline acceleration-magnitude threshold detector described in
[`docs/baseline-motion-detection.md`](../docs/baseline-motion-detection.md).
