# ARMOR Machine Learning

This folder documents the Edge Impulse machine-learning development workflow
used to build the ARMOR fall-detection classifier.

---

## Completed workflow

| Stage | Status | Notes |
|-------|--------|-------|
| Data collection | Complete | BMI160 6-axis IMU data recorded via Edge Impulse data forwarder |
| Data labelling | Complete | Three-class → two-class (see iterative training history below) |
| Feature engineering | Complete | Spectral analysis + raw axes; configured in Edge Impulse impulse designer |
| Model training | Complete | Six training iterations; final binary classifier deployed to firmware |
| Model evaluation | Complete | Tested against held-out samples in Edge Impulse; screenshots in `evidence/ml-screenshots/` |
| TinyML deployment | Complete | Exported as Arduino library; included in sender firmware |

---

## Iterative training history

The model was developed over six numbered attempts. Each attempt is documented
with its configuration, confusion matrix results, and what changed before the
next run. Screenshots are embedded inline below.

A Google Sheets log was maintained alongside Edge Impulse throughout development
to track each recorded fall, its type (backwards, forwards, left, right, wall
slide), and the specific motion performed. This provided a structured record of
sample coverage and helped identify which fall types were under-represented
before each new collection session.

---

### Attempt 1 — Initial three-class model (poor accuracy)

**Architecture:** Input (78 features) → Dense 20 → Dense 10 → Output (3 classes)  
**Labels:** `fall_like`, `fast_motion`, `normal`  
**Training cycles:** 100 | **Learning rate:** 0.0005

| Metric | Value |
|--------|-------|
| Training accuracy | 30.0% |
| Training loss | 8.72 |
| AUC | 0.64 |
| Weighted precision | 0.28 |
| Weighted recall | 0.30 |
| Weighted F1 | 0.28 |

**Confusion matrix (validation set):**

| Predicted → | `fall_like` | `fast_motion` | `normal` |
|-------------|-------------|---------------|----------|
| `fall_like` | 66.7% | 33.3% | 0% |
| `fast_motion` | 60% | 20% | 20% |
| `normal` | 0% | 0% | 100% |

**Outcome:** The model almost entirely collapsed predictions into the `normal`
class for `fast_motion` samples, and confused a large portion of `fall_like`
with `fast_motion`. The data explorer showed heavy overlap between all three
classes with no clear separation. The overall 30% accuracy confirmed the initial
dataset was too small to train a reliable three-class boundary.

**Action taken:** More labelled samples collected across all three classes before
the next run.

![Attempt 1 — training output and confusion matrix](../evidence/ml-screenshots/01-attempt1-training.png)

---

### Attempt 2 — More data added, three-class model improves

**Architecture:** Input (78 features) → Dense 20 → Dense 10 → Output (3 classes)  
**Labels:** `fall_like`, `fast_motion`, `normal`

| Metric | Value (model testing, float32) |
|--------|-------------------------------|
| Test accuracy | 66.67% |
| AUC | 0.83 |
| Weighted precision | 0.75 |
| Weighted recall | 0.67 |
| Weighted F1 | 0.68 |

**Confusion matrix (model testing):**

| Predicted → | `fall_like` | `fast_motion` | `normal` |
|-------------|-------------|---------------|----------|
| `fall_like` | 60% | 40% | 0% |
| `fast_motion` | 20% | 60% | 0% |
| `normal` | 0% | 40% | 60% |

**Outcome:** Adding more data produced a meaningful improvement — accuracy more
than doubled and AUC jumped to 0.83. However, `fall_like` and `fast_motion`
still frequently confused each other in both directions. `normal` samples also
misclassified as `fast_motion` at a high rate. The three-class boundary remained
unstable.

**Action taken:** Further data collected; another training run performed.

![Attempt 2 — model testing output and confusion matrix](../evidence/ml-screenshots/02-attempt2-model-testing.png)

---

### Attempt 3 — Further data, three-class ceiling reached

**Architecture:** Input (78 features) → Dense 20 → Dense 10 → Output (3 classes)  
**Labels:** `fall_like`, `fast_motion`, `normal`

| Metric | Value (validation set) |
|--------|------------------------|
| Training accuracy | 67.9% |
| Training loss | 3.14 |
| AUC | 0.82 |
| Weighted precision | 0.80 |
| Weighted recall | 0.68 |
| Weighted F1 | 0.66 |

**Outcome:** Accuracy was essentially flat compared to Attempt 2 (67.9% vs
66.67%), despite more data. Precision improved but recall remained low. The
`fast_motion`/`normal` boundary in particular was still weak: both classes share
similar low-to-moderate acceleration magnitudes and the model could not
consistently separate them. Adding more samples of the same three-class
structure was not improving the core problem.

**Decision:** The two non-fall classes (`normal` and `fast_motion`) were merged
into a single `non_fall` label. From the perspective of the alert system, the
only meaningful distinction is fall vs not-fall. Collapsing to a binary problem
gave the model a single, well-defined boundary to learn.

![Attempt 3 — training output and confusion matrix](../evidence/ml-screenshots/03-attempt3-training.png)

---

### Attempt 4 — Two-class binary model, major accuracy jump

**Architecture:** Input (48 features) → Dense 20 → Dense 10 → Output (2 classes)  
**Labels:** `fall_like`, `non_fall`  
**Note:** The input reduced from 78 to 48 features after the impulse was
reconfigured for the two-class dataset.

| Metric | Value (validation set) |
|--------|------------------------|
| Training accuracy | 90.3% |
| Training loss | 0.21 |
| AUC | 0.88 |
| Weighted precision | 0.92 |
| Weighted recall | 0.90 |
| Weighted F1 | 0.90 |

**Confusion matrix (validation set):**

| Predicted → | `fall_like` | `non_fall` |
|-------------|-------------|------------|
| `fall_like` | 79% | 21% |
| `non_fall` | 0% | 100% |

**Outcome:** Collapsing to binary classification produced the largest single
improvement of the entire development cycle — accuracy jumped from ~68% to
90.3% and loss fell from 3.14 to 0.21. `non_fall` precision was perfect on the
validation set. The 21% miss rate on `fall_like` (false negatives) was the
remaining concern.

**Action taken:** Model deployed to the sender microcontroller for on-device
testing to assess real-world behavior.

**On-device finding:** While the model performed well for falls, it produced
false positives during sitting-down transitions. The act of sitting down
generates a motion signature — a downward acceleration followed by settling —
that the model had not seen enough examples of during training.

**Action taken:** Additional samples of sitting up and sitting down were
re-collected and added to the `non_fall` dataset. New training run performed.

![Attempt 4 — training output and confusion matrix](../evidence/ml-screenshots/04-attempt4-training.png)

---

### Attempt 5 — Re-collected sitting data, architecture unchanged

**Architecture:** Input (48 features) → Dense 20 → Dense 10 → Output (2 classes)
**Labels:** `fall_like`, `non_fall`
**Dataset:** `non_fall` samples now include re-labelled `normal` and `fast_motion`
sub-groups plus the additional sitting samples collected after on-device testing.

| Metric | Value (model testing, float32) |
|--------|-------------------------------|
| Test accuracy | 92.31% |
| AUC | 0.88 |
| Weighted precision | 0.93 |
| Weighted recall | 0.92 |
| Weighted F1 | 0.92 |

**Confusion matrix (model testing):**

| Predicted → | `fall_like` | `non_fall` |
|-------------|-------------|------------|
| `fall_like` | 79% | 21% |
| `non_fall` | 0% | 100% |
| F1 score | 0.86 | 0.95 |

**Outcome:** Sitting false positives resolved. `non_fall` recall reached 100%
on the test set. However, the 21% miss rate on `fall_like` remained — the
architecture (Dense 20 → Dense 10) and hyperparameters (100 cycles, lr 0.0005)
had reached their limit with this dataset size. A more expressive architecture
with more training cycles was needed to push `fall_like` recall higher.

**Action taken:** Architecture expanded and hyperparameters tuned for the final
training run.

![Attempt 5 — model testing output and confusion matrix](../evidence/ml-screenshots/05-attempt5-model-testing.png)

---

### Attempt 6 — Expanded architecture, final deployed model

**Architecture:** Input (48 features) → Dense **64** → Dense **32** → **Dropout (0.3)** → Output (2 classes)
**Labels:** `fall_like`, `non_fall`
**Training cycles:** 300 | **Learning rate:** 0.003
**Dataset:** 441 training samples, 39 test samples (14m 48s total; 93/7% split)

The architecture was expanded from two small dense layers to two larger layers
with a dropout layer to reduce overfitting. Training cycles tripled and the
learning rate increased sixfold to give the larger network enough training to
converge.

| Metric | Value (validation set, quantized int8) |
|--------|----------------------------------------|
| Training accuracy | 91.0% |
| Training loss | 0.26 |
| AUC | 0.91 |
| Weighted precision | 0.91 |
| Weighted recall | 0.91 |
| Weighted F1 | 0.91 |

**Confusion matrix (validation set):**

| Predicted → | `fall_like` | `non_fall` |
|-------------|-------------|------------|
| `fall_like` | **88.4%** | 11.6% |
| `non_fall` | 6.5% | **93.5%** |
| F1 score | 0.90 | 0.91 |

**Outcome:** The expanded architecture produced the best results of the entire
development cycle. `fall_like` recall improved from 79% to 88.4% — the biggest
remaining weakness from Attempt 5. `non_fall` recall dropped slightly from 100%
to 93.5%, introducing a small false-positive rate (6.5%), but this is acceptable
given the on-device consecutive-window confirmation gate. All four aggregate
metrics (AUC, precision, recall, F1) reached 0.91. The data explorer shows
meaningfully better class separation than any earlier attempt.

**This model was exported as the final Arduino library and deployed to the
sender firmware.**

![Attempt 6 — training output and confusion matrix](../evidence/ml-screenshots/06-attempt6-training.png)

![Attempt 6 — dataset overview](../evidence/ml-screenshots/06-attempt6-dataset.png)

![Attempt 6 — feature explorer and on-device performance](../evidence/ml-screenshots/06-attempt6-features.png)

![Attempt 6 — impulse design](../evidence/ml-screenshots/06-attempt6-impulse.png)

![Attempt 6 — spectral features parameters](../evidence/ml-screenshots/06-attempt6-spectral.png)

---

## Label and architecture history summary

| Attempt | Classes | Architecture | Accuracy | Key change |
|---------|---------|--------------|----------|------------|
| 1 | 3 | Dense 20 → 10 | 30.0% | Initial run; dataset too small |
| 2 | 3 | Dense 20 → 10 | 66.67% | More data added |
| 3 | 3 | Dense 20 → 10 | 67.9% | More data; accuracy plateaued |
| 4 | 2 | Dense 20 → 10 | 90.3% | Merged non-fall classes; large jump |
| 5 | 2 | Dense 20 → 10 | 92.31% | Re-collected sitting data; false positives resolved |
| **6** | **2** | **Dense 64 → 32 → Dropout 0.3** | **91.0%** | **Expanded architecture; fall_like recall 79% → 88.4%; deployed** |

---

## Final model configuration (Attempt 6)

### Dataset

| Parameter | Value |
|-----------|-------|
| Total data collected | 14m 48s |
| Training samples | 441 windows |
| Test samples | 39 samples |
| Dataset split | 93% train / 7% test |
| Sample length | 2 s per window |
| Classes | `fall_like`, `non_fall` |

### Impulse design

| Parameter | Value |
|-----------|-------|
| Input axes | ax, ay, az, gx, gy, gz (6-axis) |
| Window size | 2,000 ms |
| Window stride | 500 ms |
| Frequency | 50 Hz |
| Zero-pad data | Yes |
| Processing block | Spectral Analysis |
| Learning block | Classifier (neural network) |

### Spectral features parameters

| Parameter | Value |
|-----------|-------|
| Scale axes | 0.000085932800054996992 |
| Input decimation ratio | 1 |
| Filter type | Low-pass |
| Cut-off frequency | 1.26953125 Hz |
| Filter order | 6 |
| Analysis type | FFT |
| FFT length | 128 |
| Take log of spectrum | Yes |
| Overlap FFT frames | No |
| Improve low frequency resolution | No |
| On-device processing time | 18 ms |
| Peak RAM (DSP) | 4 KB |

### Neural network

| Parameter | Value |
|-----------|-------|
| Platform | Edge Impulse |
| Model type | Neural network binary classifier |
| Architecture | Input (48) → Dense 64 → Dense 32 → Dropout (0.3) → Output (2) |
| Output classes | `fall_like`, `non_fall` |
| Training cycles | 300 |
| Learning rate | 0.003 |
| Validation set size | 20% |
| Batch size | 32 |
| Exported format | Arduino library (`.zip`) — quantized int8 |
| Library name | `armor-fall-detection-binary-v1` |
| On-device inference time | 1 ms |
| Peak RAM (inference) | 1.4 KB |

---

## On-device inference settings

| Parameter | Value |
|-----------|-------|
| `FALL_LIKE_THRESHOLD` | 0.80 |
| `REQUIRED_CONSECUTIVE_FALLS` | 3 |
| Effective confirmation window | 3 × 500 ms = 1.5 s sustained fall-like signal |
| Alert cooldown | 5 s after any trigger |

---

## Deployed library

The trained model is exported from Edge Impulse as an Arduino `.zip` library
and included in the sender firmware via:

```cpp
#include <armor-fall-detection-binary-v1_inferencing.h>
```

To retrain or update the model, export a new Arduino library from Edge Impulse,
replace the library in the Arduino IDE libraries folder, and re-flash the sender.
