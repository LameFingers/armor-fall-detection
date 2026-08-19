# ARMOR Baseline Motion Detection

This document explains how the current ARMOR prototype detects motion events,
the limitations of the approach, and the planned path toward a more reliable
fall-detection system.

---

## What the current system does

The current ARMOR prototype implements a **baseline acceleration-magnitude
threshold motion alert**.  It is **not a validated medical or safety-certified
fall detector**.

The BMI160 accelerometer reports three-axis acceleration data.  The firmware
calculates the total magnitude:

```
magnitude = sqrt(ax² + ay² + az²)
```

When `magnitude >= MOTION_THRESHOLD_G` and the alert cooldown has elapsed,
the firmware treats the event as a potential fall and transmits a `FALL_ALERT`
LoRa packet.

---

## Gravity at rest

The BMI160 is a MEMS inertial sensor that measures acceleration relative to
free fall. At complete rest the sensor is subjected only to gravitational
acceleration — approximately **9.8 m/s²** or **1 g** — distributed across its
three axes depending on orientation.

> **At rest, the measured acceleration magnitude should be near 1 g because
> gravity is always included in the measurement.**

This means:

- Readings consistently below ~0.9 g or above ~1.1 g at rest indicate sensor
  noise, calibration bias, or vibration.
- Small per-axis variation at rest is normal and is due to sensor noise and
  offset bias.  This is expected behavior, not a fault.
- A threshold of, for example, 1.2 g means the firmware alerts on any motion
  that adds as little as 0.2 g above the resting gravitational baseline.

---

## Threshold calibration

The default threshold of `1.2 g` is a **conservative starting point**.  It
will generate false positives during normal arm movements and walking.

**Calibration procedure:**

1. Place the sender unit on a flat surface and record the resting magnitude
   from the Serial Monitor.  It should be close to 1.0 g.
2. Perform the target activity (e.g. a controlled sharp movement or a simulated
   fall motion with soft padding) and record the peak magnitude.
3. Set `MOTION_THRESHOLD_G` to a value between the peak resting magnitude and
   the peak event magnitude to separate the two populations.
4. Re-test and repeat until false positives are minimised and genuine events
   are reliably detected.

> A single acceleration threshold applied to magnitude alone may cause
> **false positives** (ordinary movements exceeding the threshold) and may
> **miss low-acceleration events** (slow falls or gradual collapses).
> Controlled testing is essential before relying on any threshold value.

---

## Known limitations

- A single scalar threshold on acceleration magnitude is a very simple
  classifier.  Real falls produce characteristic acceleration patterns that
  this detector cannot distinguish from other sharp movements.
- The system has no cancellation mechanism — the wearer cannot stop a false
  alert after it is triggered.
- LoRa range and reliability have not been field-tested.
- The prototype has not been evaluated for battery life, robustness, or
  wearability.

---

## False positives and missed detections

Track every test session in [`docs/testing-log-template.md`](testing-log-template.md):

| Event type        | Definition                                              |
|-------------------|---------------------------------------------------------|
| False positive    | Alert triggered by non-fall motion (e.g. arm swing, placing device down) |
| Missed detection  | A controlled fall-like event that did NOT trigger the alert |

Use these counts to guide threshold adjustment.

---

## Future improvements

The following enhancements are planned for future development iterations:

| Area                                | Description                                                   |
|-------------------------------------|---------------------------------------------------------------|
| Gyroscope / orientation features    | Use angular rate and orientation to distinguish fall patterns |
| Post-impact stillness detection     | Detect the period of inactivity that typically follows a real fall |
| Cancellation button / timeout       | Allow the wearer to cancel a false alarm within a timeout window |
| Barometric sensing                  | Detect rapid altitude change as a secondary fall indicator    |
| Dataset creation                    | Record labeled BMI160 data for normal activities and fall-like events |
| Feature extraction                  | Compute windowed features (peak magnitude, jerk, orientation change, energy) |
| TinyML model training               | Train a classifier on labeled data using Edge Impulse or equivalent |
| TinyML deployment                   | Deploy the model to the ESP32 for on-device inference          |

> This system is a **working baseline prototype** intended for controlled
> development testing only.  It is not medically reliable and must not be
> used as a safety system without extensive validation.
