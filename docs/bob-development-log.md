# IBM Bob Development Log

## Purpose

This log documents how IBM Bob is used as Armor's primary AI development
partner during planning, implementation, testing, debugging, and documentation.

Each entry records:
- The development objective
- The prompt or task given to IBM Bob
- Bob's contribution
- The final decision made by the project owner
- The files, code, tests, or design artifacts produced

All IBM Bob suggestions are reviewed, adapted, and tested before use.

---

## Entry 001 — Hardware specification correction and documentation update

**Date:** 2026
**Objective:** Correct all project documentation to reflect the actual hardware
on hand and align the repository with the real bill of materials before any
firmware or ML work begins.

**Prompt given to Bob:**
> The docs refer to BMI270 and a generic ESP32-S3. The actual hardware is a
> HiLetgo BMI160 IMU and two LilyGO LoRa32 915 MHz boards (TTGO LoRa32 /
> TTGO Paxcounter: ESP32 + SX1276/SX1278 + OLED + SD + BLE + Wi-Fi).
> Update every part of the documentation accordingly and log it here. Ignore
> pin assignments for now as parts have not yet arrived.

**Bob's contribution:**
Bob audited all nine repository files, identified every reference to the
incorrect components, and rewrote the affected sections across the following
files:

| File | Changes made |
|------|-------------|
| `README.md` | Replaced BMI270 / ESP32-S3 with BMI160 / LilyGO LoRa32; updated components list; changed "gateway" to "base station" |
| `docs/architecture.md` | Rewrote overview, information-flow steps, and subsystems to reflect two LilyGO LoRa32 boards communicating over a direct 915 MHz LoRa link; added board capability note (OLED, SD, BLE, Wi-Fi unused in v1) |
| `firmware/README.md` | Added board-specification table; split firmware into two targets (wearable and base station); updated library list to BMI160-Arduino, RadioLib/arduino-LoRa, Adafruit SSD1306 |
| `hardware/README.md` | Replaced generic component list with a full BOM table using actual part names; updated development sequence; added 915 MHz frequency/regulatory note |
| `ml/README.md` | Updated IMU name to BMI160; added BMI160 sensor range details; added deployment-platform section with ESP32 memory constraints |

Bob also noted the following design risks identified during the audit (see
initial audit response for the full ranked list):
- The SX1276/SX1278 radio variant on the LilyGO LoRa32 should be confirmed
  against the specific board revision once parts arrive (some revisions use
  SX1276, others SX1278; pin assignments differ slightly).
- The BMI160 I²C address (default 0x68 or 0x69 depending on SDO pin) should
  be confirmed on the HiLetgo breakout board.

**Project owner decision:**
Corrections accepted. Pin assignments and library confirmation deferred until
hardware arrives.

**Artifacts produced:**
- Updated `README.md`
- Updated `docs/architecture.md`
- Updated `firmware/README.md`
- Updated `hardware/README.md`
- Updated `ml/README.md`
- This log entry (`docs/bob-development-log.md`)

---

## Entry 002 — Working firmware, wiring documentation, and baseline motion detection

**Date:** July 15, 2026
**Objective:** Add physically tested sender and receiver firmware to the
repository and create a full set of supporting documentation covering
hardware wiring, firmware setup, motion detection methodology, and
structured test logging.

**Prompt given to Bob:**
> Provided two physically tested Arduino sketches — sender (LILYGO T3 V1.6.1 +
> DFRobot Gravity BMI160 + RGB LED + KY-006 + LoRa) and receiver (LILYGO T3
> V1.6.1 + RGB LED + KY-006 + LoRa). Bob was instructed to create the two
> `.ino` files with comments only (no executable changes), create seven
> documentation and README files, and add a `## Current Working Prototype`
> section to `README.md` without removing any existing content.

**Bob's contribution:**
Bob created both firmware files with engineering comments added throughout,
and authored all requested documentation files:

| File | Action | Description |
|------|--------|-------------|
| `firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino` | Created | Sender firmware with comments; 492 lines |
| `firmware/receiver_lora_alert/receiver_lora_alert.ino` | Created | Receiver firmware with comments; 399 lines |
| `docs/hardware-wiring.md` | Created | GPIO pin assignments, I²C addresses, RGB LED, KY-006, and power wiring for both boards |
| `docs/firmware-setup.md` | Created | Arduino IDE board selection, required libraries, upload steps, LoRa parameter matching, and troubleshooting |
| `docs/baseline-motion-detection.md` | Created | Explains the threshold approach, gravity-at-rest baseline, calibration guidance, known limitations, and future ML improvements |
| `docs/testing-log-template.md` | Created | Markdown table template for recording controlled test sessions with RSSI, SNR, false positives, and missed events |
| `evidence/README.md` | Updated | Replaced planning stub with guidelines on what to store, what to exclude, and file naming conventions |
| `hardware/README.md` | Updated | Replaced planning BOM with accurate v1 working prototype components; added future hardware work section |
| `ml/README.md` | Updated | Replaced planning stub; clarified folder is reserved for future data and model work; linked to baseline detection doc |
| `README.md` | Updated | Added `## Current Working Prototype` section with description, firmware table, and key documentation links |

Comments in both `.ino` files cover: project purpose and board role, library
purposes, internal SX1276 pin mapping (do-not-rewire notice), OLED I²C
address, BMI160 shared bus and address, common-anode RGB LED logic, KY-006
differential-drive wiring, configuration constants, normal and alert states,
LoRa packet format, acceleration magnitude calculation, gravity-at-rest
explanation, alert cooldown, RSSI and SNR definitions, return-to-listening
state, and the `setup()` initialization rationale.  Comments explicitly note
that a single acceleration threshold may cause false positives and that the
system is not a validated fall detector.

**Project owner decision:**
Firmware files and documentation accepted pending review of VS Code diff.
Threshold calibration, controlled testing, and data collection are the next
development priorities.

**Artifacts produced:**
- Created `firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino`
- Created `firmware/receiver_lora_alert/receiver_lora_alert.ino`
- Created `docs/hardware-wiring.md`
- Created `docs/firmware-setup.md`
- Created `docs/baseline-motion-detection.md`
- Created `docs/testing-log-template.md`
- Updated `evidence/README.md`
- Updated `hardware/README.md`
- Updated `ml/README.md`
- Updated `README.md`
- This log entry (`docs/bob-development-log.md`)
