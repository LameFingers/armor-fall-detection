# Armor System Architecture

## Overview

Armor is an edge-AI wearable safety system that detects probable falls and
alerts a remote observer over LoRa without any internet or smartphone
dependency. All motion analysis runs locally on the wearable device using a
trained TinyML classifier.

Both the wearable and the base station run on **LILYGO T3 V1.6.1** boards
(ESP32 + SX1276 LoRa + SSD1306 OLED). The wearable reads a DFRobot Gravity
BMI160 6-axis IMU over I²C. The two boards communicate directly over
915 MHz LoRa — there is no LoRaWAN gateway or cloud service.

---

## Information flow

1. The BMI160 IMU samples accelerometer and gyroscope data at **50 Hz**.
2. The sender firmware maintains a continuously rolling feature buffer of the
   most recent 2-second window (all 6 axes: ax, ay, az, gx, gy, gz).
3. Every **500 ms** the Edge Impulse binary classifier runs on the buffer and
   produces a `fall_like` confidence score (0–1).
4. If the score exceeds **0.80 for 3 consecutive windows** (1.5 s sustained),
   the sender enters a **5-second cancellation countdown**.
5. The wearer can press **GPIO 2** within 5 seconds to suppress the alert.
6. If the countdown expires without a button press, the sender transmits a
   compact `FALL_ALERT` LoRa packet and sounds the buzzer.
7. The receiver listens continuously. On receipt it activates a repeating
   buzzer alarm and turns its LED red.
8. An operator presses the **receiver GPIO 2** button to acknowledge and
   silence the alarm.

---

## Main subsystems

| Subsystem | Implementation |
|-----------|---------------|
| IMU sensing | DFRobot Gravity BMI160 (SEN0250), 6-axis, I²C address 0x69 |
| Edge processing | LILYGO T3 V1.6.1 (ESP32, 240 MHz dual-core) |
| TinyML inference | Edge Impulse binary classifier — `fall_like` vs `non_fall` |
| User feedback | KY-006 buzzer, common-anode RGB LED, GPIO 2 cancel button |
| LoRa communications | SX1276 on-board, 915 MHz, SF7, 125 kHz BW, 4/5 CR |
| Base station | Second LILYGO T3 V1.6.1 with buzzer, LED, and acknowledge button |
| Power | PKCELL LP503562 3.7 V LiPo via T3 V1.6.1 JST connector; battery ADC on GPIO 35 |
| Display | Built-in SSD1306 128×64 OLED on both boards |

---

## Packet format

```
FALL_ALERT,<alertNumber>,CONF=<confidence>
```

Example: `FALL_ALERT,3,CONF=0.94`

Only one packet type is transmitted in this version. The receiver parses the
alert number and confidence score for display on its OLED.

---

## What is not used in v1

The T3 V1.6.1 includes Wi-Fi, Bluetooth, and an SD-card slot. None of these
are used in the current firmware. All processing and communication is handled
locally by the two boards over direct LoRa.
