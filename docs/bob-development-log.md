# AI-Assisted Development Summary

This document summarises how AI tooling (IBM Bob) was used throughout the
development of the ARMOR prototype - covering system planning, firmware
implementation, hardware documentation, TinyML integration, and testing.

All AI suggestions were reviewed, adapted, and physically tested before being
accepted into the repository. The final project owner made all design decisions.

---

## Session 1 - Hardware specification and documentation alignment

**Objective:** Correct all project documentation to reflect the actual hardware
on hand and align the repository with the real bill of materials before any
firmware or ML work began.

**What was done:**
All repository files were audited for incorrect component references. Every
mention of the earlier placeholder hardware (BMI270, generic ESP32-S3) was
replaced with the actual parts: DFRobot Gravity BMI160 and LILYGO T3 V1.6.1.
Architecture, firmware, hardware, and ML documentation were all updated in a
single pass.

**Outcome:** Documentation aligned to real hardware before any code was written.

---

## Session 2 - Firmware and supporting documentation

**Objective:** Add physically tested sender and receiver firmware to the
repository with full engineering comments, and create supporting documentation
covering hardware wiring, firmware setup, motion detection, and evidence
organisation.

**What was done:**
Both firmware `.ino` files were commented throughout. Seven documentation and
README files were authored:

| File | Action |
|------|--------|
| `firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino` | Engineering comments added |
| `firmware/receiver_lora_alert/receiver_lora_alert.ino` | Engineering comments added |
| `docs/hardware-wiring.md` | Created - GPIO pin assignments, I2C addresses, LED/buzzer/power wiring |
| `docs/firmware-setup.md` | Created - Arduino IDE setup, library install, upload steps, troubleshooting |
| `docs/baseline-motion-detection.md` | Created - documents the original threshold approach and why it was replaced |
| `evidence/README.md` | Created - evidence folder structure and naming conventions |
| `hardware/README.md` | Updated - v1 working prototype BOM |
| `ml/README.md` | Updated - ML folder purpose and future work |
| `README.md` | Updated - Current Working Prototype section added |

**Outcome:** First working prototype fully documented in the repository.

---

## Session 3 - Button design consultation

**Objective:** Decide how many tactile buttons to add to the sender and receiver,
and what each should do.

**What was done:**
Design options were evaluated for the sender cancel button and receiver
acknowledge button. One button per board was recommended - the sender cancel
(suppress alert during countdown) and the receiver acknowledge (silence the
repeating alarm). A two-button receiver design with LoRa ACK-back was identified
as a valid future feature.

Final implementation used **GPIO 2** on both boards (normally-open pushbutton
to GND, INPUT_PULLUP).

**Outcome:** Single-button design adopted and implemented (see Session 4).

---

## Session 4 - Button implementation, LEDC buzzer, and battery monitoring

**Objective:** Update both firmware files with physically tested code adding
cancel/acknowledge buttons, a non-blocking repeating receiver alarm, and LiPo
battery voltage monitoring on the sender.

**What was done:**
Both firmware files and six documentation files were updated:

| File | Action |
|------|--------|
| `firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino` | Cancel button (GPIO 2), 5-second countdown, battery ADC (GPIO 35), updated OLED |
| `firmware/receiver_lora_alert/receiver_lora_alert.ino` | LEDC PWM buzzer, acknowledge button, non-blocking alarm state machine, packet parser |
| `docs/hardware-wiring.md` | Added button, differential buzzer (sender), LEDC buzzer (receiver), and battery ADC sections |
| `docs/firmware-setup.md` | Expanded troubleshooting table; updated Verifying Operation steps |
| `docs/baseline-motion-detection.md` | Updated to reflect the implemented 5-second cancel window |
| `hardware/README.md` | Added buttons, LiPo battery, corrected USB type and resistor counts |
| `README.md` | Rewrote prototype behavior description to match the countdown/alarm/acknowledge flow |

**Outcome:** Cancel and acknowledge flows validated on hardware. Battery
monitoring calibrated against a multimeter.

---

## Session 5 - TinyML integration and testing documentation

**Objective:** Replace the threshold-based motion detector with a trained Edge
Impulse TinyML classifier, finalise both firmware files, document the complete
training and validation history, and commit the trained model library.

**What was done:**
The sender firmware was updated from magnitude-threshold detection to the Edge
Impulse binary classifier pipeline. The receiver firmware was updated to match
the new `CONF=` packet field. The testing-validation document was written from
scratch to capture the full six-attempt training history.

| File | Action |
|------|--------|
| `firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino` | Replaced threshold with Edge Impulse rolling-buffer inference; added consecutive-window confirmation gate |
| `firmware/receiver_lora_alert/receiver_lora_alert.ino` | Updated packet parser and display for `CONF=` field |
| `docs/testing-validation.md` | Written - full training history, label evolution, per-attempt results, final confusion matrix, on-device validation |
| `ml/armor-fall-detection-binary-v1.zip` | Committed - trained and quantized int8 Edge Impulse Arduino library |

**Outcome:** TinyML prototype complete. Edge Impulse binary classifier
(91.0% accuracy, AUC 0.91, 88.4% fall_like recall) validated end-to-end on
hardware across all five fall types with no outstanding false positives.

---

## Session 6 - Repository audit and polish

**Objective:** Full audit of the repository for consistency, correctness, and
presentation quality before sharing publicly.

**What was done:**
- `.gitignore` replaced with an Arduino/embedded-specific template
- Project name standardised to `ARMOR` throughout all files
- LED wiring table corrected in `docs/hardware-wiring.md`
- `firmware/README.md` title corrected
- `evidence/hardware/README.md` image display fixed for GitHub rendering
- `LICENSE` file added (MIT)
- `README.md` internal tooling section removed; license section added
- This document reformatted for public readability

**Outcome:** Repository ready for public sharing.