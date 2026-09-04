# Armor

## Edge-AI Fall Detection and Long-Range Emergency Alerting for Remote Workers

Armor is a wearable safety system being developed for farmers and other people
who work alone in remote areas. It aims to detect potential falls locally using
motion sensors and TinyML, then send an emergency alert through LoRa when the
wearer cannot respond.

## The problem

A fall or serious injury can leave a remote worker unable to call for help.
Existing fall-detection devices can produce false alarms, miss genuine falls,
depend on smartphone or internet access, and raise privacy concerns by sending
personal motion data to the cloud.

## Proposed solution

Armor will use a BMI160 motion sensor and a LilyGO LoRa32 (TTGO) development
board to analyze motion data directly on the wearable device. When a possible
fall is detected, Armor will alert the wearer with a buzzer and LED and allow
time to cancel a false alarm. If there is no response, the device will send an
emergency message over LoRa to a second LilyGO LoRa32 base station.

## Current Prototype

A physically tested breadboard prototype is operational with a trained Edge
Impulse TinyML classifier running on-device.

> This is a prototype — not a validated medical fall detector and not
> safety-certified in any way.

### What the prototype does

- The **sender** (LILYGO T3 V1.6.1 + DFRobot Gravity BMI160) samples all
  6 IMU axes at 50 Hz and feeds them into a continuously rolling feature buffer.
- Every 500 ms the sender runs the **Edge Impulse binary classifier** on the
  buffer and scores a `fall_like` confidence (0–1).
- When `fall_like` confidence exceeds **0.80 for 3 consecutive windows**
  (1.5 s sustained), the sender enters a **5-second cancellation countdown**.
- During the countdown the LED turns red and the OLED shows the seconds
  remaining. Pressing the **GPIO 2 cancel button** suppresses the alert —
  no LoRa packet is sent.
- If the countdown expires without a button press, the sender increments the
  alert counter, transmits a `FALL_ALERT` packet over LoRa, and sounds the
  buzzer.
- The sender OLED shows live fall-risk percentage, battery voltage, and power
  source.
- The **receiver** (LILYGO T3 V1.6.1) listens continuously for `FALL_ALERT`
  packets.
- On receipt the receiver turns its LED red, sounds a **repeating buzzer
  alarm**, and shows the alert number, confidence score, RSSI, and SNR on its
  OLED. The alarm continues until the **GPIO 2 acknowledge button** is pressed.
- After acknowledgement the receiver LED returns to green and the OLED returns
  to `LISTENING`.

### Firmware

| Board | Sketch |
|-------|--------|
| Sender | [`firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino`](firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino) |
| Receiver | [`firmware/receiver_lora_alert/receiver_lora_alert.ino`](firmware/receiver_lora_alert/receiver_lora_alert.ino) |

### TinyML model

| Item | Location |
|------|----------|
| Trained Arduino library (`.zip`) | [`ml/armor-fall-detection-binary-v1.zip`](ml/armor-fall-detection-binary-v1.zip) |
| Model details and install instructions | [`ml/README.md`](ml/README.md) |
| Training history and validation results | [`docs/testing-validation.md`](docs/testing-validation.md) |

### Key documentation

| Document | Description |
|----------|-------------|
| [`docs/hardware-wiring.md`](docs/hardware-wiring.md) | GPIO pin assignments and breadboard wiring for both boards |
| [`docs/firmware-setup.md`](docs/firmware-setup.md) | Arduino IDE setup, library installation, and upload instructions |
| [`docs/testing-validation.md`](docs/testing-validation.md) | Training activities, iterative model results, and final validation metrics |
| [`hardware/README.md`](hardware/README.md) | Bill of materials for the prototype |
| [`ml/README.md`](ml/README.md) | ML model details, install instructions, and training history |

---

## Project status

**TinyML prototype complete.** The Edge Impulse binary classifier has been
trained over six iterations, validated on-device, and deployed to the sender
firmware. The trained model library is committed to this repository.

## IBM Bob usage

IBM Bob is being used as the primary AI development partner for system planning,
architecture, code development, test design, debugging, and project
documentation. Development evidence will be maintained in
[`docs/bob-development-log.md`](docs/bob-development-log.md).

## Safety note

Armor is an early-stage prototype and is not certified medical equipment or a
replacement for emergency services.
