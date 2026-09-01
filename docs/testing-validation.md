# Armor Testing and Validation

## Purpose

Armor must distinguish genuine falls from normal movement while minimizing false
alarms. This document records the data-collection activities, labelling
decisions, iterative training process, and final validation results used to
build the Edge Impulse TinyML binary classifier (`fall_like` vs `non_fall`).

---

## Data collection and tracking

Sensor data was recorded using the BMI160 IMU at 50 Hz via the Edge Impulse
data forwarder. A Google Sheets log was maintained throughout to track every
recorded sample — each row recorded the fall type, the specific motion
performed, and which training attempt it was added for. This made it possible
to identify which activities were under-represented before each new collection
session.

---

## Label evolution

Data collection began with three classes:

| Original label | Description |
|----------------|-------------|
| `normal` | Low-intensity everyday activities |
| `fast_motion` | Quick everyday movements that could cause false positives |
| `fall_like` | Genuine fall events |

After three training attempts the three-class model plateaued around 67–68%
accuracy. The `normal` and `fast_motion` classes shared enough IMU overlap that
the model could not draw a stable boundary between them. Because the system only
needs to distinguish fall from not-fall, the two non-fall classes were merged:

| Final label | Merged from | Description |
|-------------|-------------|-------------|
| `non_fall` | `normal` + `fast_motion` | All everyday activity regardless of speed |
| `fall_like` | `fall_like` | Genuine fall events |

---

## Training activities

### `fall_like` — genuine fall scenarios

| Activity | Description |
|----------|-------------|
| **Falling backwards** | Uncontrolled backward fall from standing position |
| **Falling forwards** | Forward fall from standing position |
| **Falling left side** | Lateral fall to the left from standing position |
| **Falling right side** | Lateral fall to the right from standing position |
| **Slow wall slide** | Controlled slide down a wall to simulate a low-energy collapse; captures falls that a magnitude threshold would miss entirely |

Each fall type produces a distinct IMU signature: a sharp acceleration spike
during impact followed by a rapid orientation change. The wall-slide was
specifically included because it generates a slow, sustained motion rather than
an abrupt impact — a pattern the earlier threshold-based detector could not
reliably detect.

### `non_fall` — everyday activities

**Normal-pace activities** (originally labelled `normal`):

| Activity | Description |
|----------|-------------|
| **Walking** | Normal-pace walking indoors |
| **Sitting still** | Remaining seated without significant movement |
| **Sitting up and down** | Transitioning between seated and standing positions |
| **Moving objects while seated** | Reaching, lifting, and repositioning objects from a chair |

**Fast activities** (originally labelled `fast_motion`):

| Activity | Why it was included |
|----------|---------------------|
| **Fast squats** | Vertical acceleration profile resembles a fall impact |
| **Fast reaches** | Sharp lateral spike resembles a lateral fall onset |
| **Repeated everyday actions** | Any motion that produced a false positive during early runs was re-collected and added until the model reliably rejected it |

After on-device testing (see iterative results below), sitting-up-and-down
samples were specifically re-collected in larger volume because sitting down
was generating false positives when the Attempt 4 model was deployed to
hardware.

---

## Iterative training results

| Attempt | Classes | Architecture | Accuracy | AUC | Notes |
|---------|---------|--------------|----------|-----|-------|
| 1 | 3 | Dense 20 → 10 | 30.0% | 0.64 | Initial dataset too small; model collapsed to `normal` |
| 2 | 3 | Dense 20 → 10 | 66.67% | 0.83 | More data added; large improvement but cross-class confusion remained |
| 3 | 3 | Dense 20 → 10 | 67.9% | 0.82 | Further data; accuracy plateaued — three-class boundary unstable |
| 4 | 2 | Dense 20 → 10 | 90.3% | 0.88 | Merged non-fall classes; large jump; on-device false positives during sitting |
| 5 | 2 | Dense 20 → 10 | 92.31% | 0.88 | Re-collected sitting data; false positives resolved |
| **6** | **2** | **Dense 64 → 32 → Dropout 0.3** | **91.0%** | **0.91** | **Expanded architecture; fall_like recall 79% → 88.4%; final deployed model** |

Full per-attempt confusion matrices and Edge Impulse screenshots are in
`evidence/ml-screenshots/`. Full model configuration details are in
[`ml/README.md`](../ml/README.md).

---

## Final model validation metrics (Attempt 6)

| Metric | Value |
|--------|-------|
| Training accuracy | 91.0% |
| Area under ROC curve | 0.91 |
| Weighted precision | 0.91 |
| Weighted recall | 0.91 |
| Weighted F1 | 0.91 |

**Confusion matrix (validation set, Attempt 6):**

| Predicted → | `fall_like` | `non_fall` |
|-------------|-------------|------------|
| `fall_like` | **88.4%** | 11.6% |
| `non_fall` | 6.5% | **93.5%** |
| F1 score | 0.90 | 0.91 |

`fall_like` recall improved from 79% (Attempt 5) to 88.4% by expanding the
network architecture and increasing training cycles. A small 6.5% false-positive
rate on `non_fall` was introduced as a tradeoff but is mitigated on-device by
the `REQUIRED_CONSECUTIVE_FALLS = 3` confirmation gate, which requires three
consecutive high-confidence windows before any alert fires.

---

## On-device validation

| Area | Result |
|------|--------|
| Fall detection (all 5 fall types) | Validated on-device |
| False positives during sitting still | Resolved after Attempt 5 re-collection |
| False positives during sitting up and down | Resolved after Attempt 5 re-collection |
| Cancel-button suppression (5-second countdown) | Validated on-device |
| LoRa transmission and packet receipt | Validated on-device (sender → receiver) |
| OLED display and alert sequence | Validated on-device |
| Battery monitoring accuracy | Validated against multimeter reading |

---

## Safety note

Armor is a prototype and not a medical or safety-certified device. All fall
tests were performed safely with appropriate precautions. Results must not be
presented as clinical validation or regulatory compliance.
