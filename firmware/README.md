# ARMOR Firmware

This folder contains the embedded firmware for both ARMOR boards.

---

## Sketches

| Sketch | Board | Description |
|--------|-------|-------------|
| [`sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino`](sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino) | LILYGO T3 V1.6.1 (wearable) | Reads BMI160 IMU, runs TinyML inference, triggers LoRa alert after cancellation countdown |
| [`receiver_lora_alert/receiver_lora_alert.ino`](receiver_lora_alert/receiver_lora_alert.ino) | LILYGO T3 V1.6.1 (base station) | Listens for FALL_ALERT packets, sounds repeating alarm, waits for acknowledge button |

---

## Board

Both sketches target the **LILYGO T3 V1.6.1**:

| Feature | Detail |
|---------|--------|
| SoC | ESP32 (dual-core Xtensa LX6, 240 MHz) |
| LoRa radio | SX1276 @ 915 MHz (hardwired on PCB) |
| Display | Built-in SSD1306 128×64 OLED |
| Battery | 3.7 V LiPo via JST connector |

---

## Required libraries

Install via **Sketch → Include Library → Manage Libraries** or **Add .ZIP Library**:

| Library | Source | Used by |
|---------|--------|---------|
| `LoRa` by Sandeep Mistry | Library Manager | Both |
| `Adafruit GFX Library` | Library Manager | Both |
| `Adafruit SSD1306` | Library Manager | Both |
| `DFRobot_BMI160` | Library Manager | Sender only |
| `armor-fall-detection-binary-v1` | [`ml/armor-fall-detection-binary-v1.zip`](../ml/armor-fall-detection-binary-v1.zip) | Sender only |

For full setup and upload instructions see [`docs/firmware-setup.md`](../docs/firmware-setup.md).

---

## LoRa parameters

Both sketches must use identical radio parameters or packets will not be decoded:

| Parameter | Value |
|-----------|-------|
| Frequency | 915 MHz |
| Spreading factor | 7 |
| Signal bandwidth | 125 kHz |
| Coding rate | 4/5 |
| TX power (sender) | 17 dBm |
