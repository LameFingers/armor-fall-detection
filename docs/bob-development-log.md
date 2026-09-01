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

---

## Entry 003 — Button design consultation: sender cancel and receiver acknowledge

**Objective:** Determine how many tactile buttons to add to the sender and
receiver boards, and what each button should do.

**Prompt given to Bob:**
> I want to add a button to the sender so that if a fall is detected, the user
> can press it to cancel the alert in case they fell but are well enough to get
> medical attention themselves. I'm debating on only adding one button to the
> receiver. How many buttons should I add to either board and why?

**Bob's recommendation:**

*Sender — 1 button ("I'm OK" / cancel):*
One button with one function — cancel the alert. During the alert window the
wearer presses it to suppress the LoRa transmission. More buttons add
complexity that a person in distress may not be able to use. Recommended GPIO:
**GPIO 0** (onboard BOOT button on the T3 V1.6.1, no extra wiring required).

*Receiver — 1 button (acknowledge / silence):*
One button to silence the repeating alarm and confirm the alert has been seen.
A second receiver button (e.g. send a LoRa ACK back to the sender) was
identified as a valid future feature, but deferred until the sender firmware
has a handler for incoming packets.

**Button summary:**

| Board    | Buttons | GPIO       | Function                                             |
|----------|---------|------------|------------------------------------------------------|
| Sender   | 1       | GPIO 0 (onboard BOOT — no extra wiring) | Cancel / "I'm OK" |
| Receiver | 1       | GPIO 34 (or any free input GPIO) | Acknowledge / silence |

**Project owner decision:**
Design accepted. Button GPIO was changed to **GPIO 2** on both boards in the
actual implementation (see Entry 004). No files were modified in this session.

**Artifacts produced:**
- This log entry (`docs/bob-development-log.md`)

---

## Entry 004 — Button implementation, receiver LEDC buzzer, and battery monitoring

**Objective:** Update both firmware files to match physically tested code that
adds cancel/acknowledge buttons, a non-blocking repeating receiver alarm, and
LiPo battery voltage monitoring on the sender.

**Prompt given to Bob:**
> Provided two updated, physically tested Arduino sketches — sender with a
> 5-second cancel countdown (GPIO 2 button), battery ADC monitoring (GPIO 35),
> and updated OLED layout; receiver with a repeating non-blocking LEDC buzzer
> alarm, GPIO 2 acknowledge button, per-packet parsing, and packet/alert
> counters. Instructed to update firmware with comments only, then update all
> appropriate repository files and add a development log entry.

**Bob's contribution:**
Bob replaced both firmware files (comments improved to match new behavior;
executable code preserved exactly) and updated six other repository files:

| File | Action | Description |
|------|--------|-------------|
| `firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino` | Updated | 729 lines — cancel button, battery monitoring, countdown OLED screens |
| `firmware/receiver_lora_alert/receiver_lora_alert.ino` | Updated | 635 lines — LEDC PWM buzzer, acknowledge button, non-blocking alarm sequencer, packet parser |
| `docs/hardware-wiring.md` | Updated | Added cancel/acknowledge button wiring section; split buzzer section to document sender differential vs receiver LEDC drive; added battery ADC section (GPIO 35) |
| `docs/firmware-setup.md` | Updated | Updated Verifying operation steps to reflect 5-second countdown and button interactions; expanded troubleshooting table with button and battery rows |
| `docs/baseline-motion-detection.md` | Updated | Replaced "no cancellation mechanism" limitation with description of implemented cancel window; updated detection flow to describe the 5-second countdown |
| `hardware/README.md` | Updated | Added buttons and LiPo battery to BOM; corrected USB to Micro-USB; corrected resistor count note |
| `README.md` | Updated | Rewrote Current Working Prototype behavior bullets to describe cancel countdown, repeating receiver alarm, and acknowledge button |

**Key behavioral changes documented:**

*Sender:*
- On threshold trigger, enters a 5-second OLED countdown rather than
  immediately transmitting.
- GPIO 2 (normally-open pushbutton → GND, INPUT_PULLUP) cancels the alert;
  no LoRa packet is sent.
- If countdown expires: alert number incremented, FALL_ALERT packet
  transmitted, buzzer sounds, then returns to green/monitoring.
- Cooldown timer starts at the beginning of the countdown window (not after
  transmission) to prevent repeated countdowns from one sustained event.
- GPIO 35 ADC reads battery voltage every 5 seconds; voltage-to-percentage
  lookup table covers 3.30–4.20 V; OLED shows voltage, percentage, and
  USB/battery power source.

*Receiver:*
- Buzzer is now driven by ESP32 hardware LEDC PWM on GPIO 12 (Core 3.x API:
  `ledcAttach`, `ledcWriteTone`, `ledcWrite` with GPIO directly). GPIO 13
  held permanently LOW as the piezo return path.
- Alarm repeats in a non-blocking tone1 → tone2 → silence state machine
  so LoRa packets continue to be received during an active alarm.
- GPIO 2 acknowledge button (INPUT_PULLUP, debounced) stops the alarm and
  returns the OLED to LISTENING.
- Additional FALL_ALERT packets received during an active alarm refresh the
  OLED with updated alert number, RSSI, and SNR without restarting the alarm.
- `receivedPacketCount` and `receivedAlertCount` tracked separately.
- Packet parser extracts alert number and magnitude from the packet string.

**Project owner decision:**
Firmware and documentation accepted pending review of VS Code diff.

**Artifacts produced:**
- Updated `firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino`
- Updated `firmware/receiver_lora_alert/receiver_lora_alert.ino`
- Updated `docs/hardware-wiring.md`
- Updated `docs/firmware-setup.md`
- Updated `docs/baseline-motion-detection.md`
- Updated `hardware/README.md`
- Updated `README.md`
- This log entry (`docs/bob-development-log.md`)

---

## Entry 005 — TinyML integration, firmware finalisation, and testing documentation

**Objective:** Replace the threshold-based motion detector with a trained Edge
Impulse TinyML model, finalise both firmware files for the GitHub repository,
update the testing-validation document to reflect the completed data-collection
and labelling sessions, and add this log entry.

**Prompt given to Bob:**
> Provided the final TinyML sender firmware incorporating the Edge Impulse
> binary classifier (fall_like vs non_fall), a 6-axis rolling feature buffer,
> consecutive-window confirmation logic, and an updated OLED layout. Instructed
> Bob to fix the firmware, clean out empty structures, and make everything look
> polished for the final GitHub state. Then provided descriptions of the actual
> training activities and instructed Bob to update the testing-validation
> document and add a development log entry with dates removed from all entries.

**Bob's contribution:**
Bob finalised both firmware files and rewrote the testing-validation document:

| File | Action | Description |
|------|--------|-------------|
| `firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino` | Updated | Replaced threshold-based detection with Edge Impulse TinyML pipeline; updated header, constants, state variables, OLED labels, packet format (MAG= → CONF=), and loop() inference logic; removed stale baseline comments |
| `firmware/receiver_lora_alert/receiver_lora_alert.ino` | Updated | Updated packet parser from MAG= to CONF=; renamed magnitude variable to confidence; updated showActiveAlert() to display confidence percentage; removed stale baseline-detector comments |
| `docs/testing-validation.md` | Updated | Replaced planning stub with documented training activities, fall categories, non-fall categories, and validation methodology |
| `docs/bob-development-log.md` | Updated | Removed date lines from all entries; added this entry |

**Key firmware changes documented:**

*Sender:*
- Added `#include <armor-fall-detection-binary-v1_inferencing.h>` for the
  Edge Impulse exported library.
- Rolling 6-axis feature buffer (`featureBuffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE]`)
  updated at 20 ms intervals; buffer filled after 100 warm-up samples.
- Classifier runs every 500 ms via `run_classifier()`; `fall_like` label score
  extracted from `result.classification[]`.
- `FALL_LIKE_THRESHOLD = 0.8` and `REQUIRED_CONSECUTIVE_FALLS = 3` gate alerts
  to require sustained high-confidence windows before triggering.
- `consecutiveFallWindows` counter reset on any sub-threshold window, on entry
  to `triggerAlert()`, and on buffer clear after an alert.
- LoRa packet field changed from `,MAG=` to `,CONF=` to reflect the confidence
  score payload.
- OLED monitoring screen updated: `Acc: Xg T:Xg` → `FallRisk:X% T:X%`.

*Receiver:*
- `parseMagnitude()` → `parseConfidence()`; searches for `CONF=` (5-char prefix).
- `latestMagnitude` → `latestConfidence`; displayed as a percentage on the
  active-alert OLED screen alongside RSSI and SNR.
- Serial log on packet receipt now prints confidence score and RSSI.

**Project owner decision:**
Firmware and documentation accepted. Repository is in final state for submission.

**Artifacts produced:**
- Updated `firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino`
- Updated `firmware/receiver_lora_alert/receiver_lora_alert.ino`
- Updated `docs/testing-validation.md`
- This log entry (`docs/bob-development-log.md`)
