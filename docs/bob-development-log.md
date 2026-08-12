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

**Date:** 2025
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
