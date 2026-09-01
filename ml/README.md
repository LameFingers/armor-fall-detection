# ARMOR Machine Learning

This folder contains the trained Edge Impulse model for the ARMOR fall-detection
classifier.

---

## Contents

| File | Description |
|------|-------------|
| `armor-fall-detection-binary-v1.zip` | Edge Impulse Arduino library export — the trained and quantized (int8) binary classifier ready to install in the Arduino IDE |

---

## How to install the library

1. Open the Arduino IDE.
2. Go to **Sketch → Include Library → Add .ZIP Library…**
3. Select `armor-fall-detection-binary-v1.zip` from this folder.
4. The library will be available as:

```cpp
#include <armor-fall-detection-binary-v1_inferencing.h>
```

This include is already present in the sender firmware at
[`firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino`](../firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino).

---

## Model summary

| Parameter | Value |
|-----------|-------|
| Platform | Edge Impulse |
| Model type | Neural network binary classifier |
| Architecture | Input (48) → Dense 64 → Dense 32 → Dropout (0.3) → Output (2) |
| Labels | `fall_like`, `non_fall` |
| Input axes | ax, ay, az, gx, gy, gz (6-axis IMU) |
| Window size | 2,000 ms at 50 Hz |
| Window stride | 500 ms |
| Training cycles | 300 |
| Learning rate | 0.003 |
| Training accuracy | 91.0% |
| AUC / Precision / Recall / F1 | 0.91 / 0.91 / 0.91 / 0.91 |
| On-device inference time | 1 ms |
| Peak RAM (inference) | 1.4 KB |
| Exported format | Quantized int8 Arduino library |

---

## On-device inference settings (sender firmware)

| Parameter | Value |
|-----------|-------|
| `FALL_LIKE_THRESHOLD` | 0.80 |
| `REQUIRED_CONSECUTIVE_FALLS` | 3 |
| Effective confirmation window | 3 × 500 ms = 1.5 s sustained fall-like signal |
| Alert cooldown | 5 s after any trigger |

---

## Training history

The full iterative training history — six attempts, three-class to two-class
consolidation, on-device false-positive feedback, architecture changes, and
final confusion matrices — is documented in:

- [`docs/testing-validation.md`](../docs/testing-validation.md)

---

## Retraining

To retrain or update the model:

1. Log into [Edge Impulse](https://studio.edgeimpulse.com) and open the ARMOR project.
2. Add new samples, adjust the impulse, or tune the architecture.
3. Export a new Arduino library (`.zip`).
4. Replace `armor-fall-detection-binary-v1.zip` in this folder.
5. Re-install the library in the Arduino IDE and re-flash the sender.
