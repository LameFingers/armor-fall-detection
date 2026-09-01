# ARMOR Baseline Motion Detection — Historical Reference

> **This document describes the original threshold-based motion detector that
> was used during early prototype development. It is kept for reference only.
> The current firmware uses the Edge Impulse TinyML binary classifier
> (`fall_like` vs `non_fall`) documented in
> [`docs/testing-validation.md`](testing-validation.md).**

---

## What the original system did

The first ARMOR prototype used a single acceleration magnitude threshold as a
motion detector. The BMI160 accelerometer's three-axis readings were combined
into a scalar magnitude:

```
magnitude = sqrt(ax² + ay² + az²)
```

When `magnitude >= MOTION_THRESHOLD_G` and the alert cooldown had elapsed, the
firmware entered a **5-second cancellation window**. If the wearer did not press
GPIO 2 within that window, a `FALL_ALERT` LoRa packet was transmitted.

---

## Why it was replaced

A single scalar threshold on magnitude is a simple classifier with significant
limitations:

- It cannot distinguish genuine falls from ordinary sharp movements (fast arm
  swings, squats, placing the device down).
- It misses low-acceleration events such as slow slides or collapses.
- Threshold calibration required repeated manual testing with no generalisation.

After three training iterations with this detector producing unacceptable false
positives at any usable threshold, the system was replaced with a trained Edge
Impulse binary neural network classifier that uses the full 6-axis IMU signal
over a 2-second rolling window. See the training history in
[`docs/testing-validation.md`](testing-validation.md).
