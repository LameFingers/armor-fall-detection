# ARMOR Firmware Setup Guide

This guide explains how to install the required tools and libraries, open the
ARMOR firmware sketches, and upload them to both LILYGO T3 V1.6.1 boards
using the Arduino IDE.

---

## Arduino IDE board selection

1. Open Arduino IDE.
2. Go to **Tools → Board → ESP32 Arduino** and select **ESP32 Dev Module**.
3. Set **Tools → Upload Speed** to `921600` (or lower if uploads fail).
4. Set **Tools → Serial Monitor baud rate** to `115200`.

> The LILYGO T3 V1.6.1 uses an ESP32. Select **ESP32 Dev Module** if a
> dedicated LILYGO T3 board entry is not available in your board package.

---

## Required libraries

Install via **Sketch → Include Library → Manage Libraries**:

| Library | Author | Purpose |
|---------|--------|---------|
| **LoRa** | Sandeep Mistry | SX1276 LoRa packet radio driver |
| **Adafruit GFX Library** | Adafruit | Graphics primitives for the OLED |
| **Adafruit SSD1306** | Adafruit | SSD1306 OLED display driver |
| **DFRobot_BMI160** | DFRobot | BMI160 6-axis IMU driver (sender only) |

The Edge Impulse library must be installed separately via
**Sketch → Include Library → Add .ZIP Library** using the file at
[`ml/armor-fall-detection-binary-v1.zip`](../ml/armor-fall-detection-binary-v1.zip).
This library is **sender only**.

---

## Opening the sketches

The two firmware sketches are located in the repository at:

- `firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino`
- `firmware/receiver_lora_alert/receiver_lora_alert.ino`

Open each sketch in a separate Arduino IDE window:
**File → Open** → navigate to the `.ino` file.

---

## Uploading

### Sender board

1. Connect the sender LILYGO T3 V1.6.1 to your computer via USB.
2. In Arduino IDE: **Tools → Port** → select the correct COM port for the
   sender board.
3. Open `firmware/sender_bmi160_lora_alert/sender_bmi160_lora_alert.ino`.
4. Click **Verify / Compile** (✓) to confirm no errors.
5. Click **Upload** (→) to flash the firmware.
6. Open **Tools → Serial Monitor** at **115200 baud** to verify startup output.

### Receiver board

1. Connect the receiver LILYGO T3 V1.6.1 to your computer via USB.
2. In Arduino IDE: **Tools → Port** → select the correct COM port for the
   receiver board.
3. Open `firmware/receiver_lora_alert/receiver_lora_alert.ino`.
4. Click **Verify / Compile** (✓) to confirm no errors.
5. Click **Upload** (→) to flash the firmware.
6. Open **Tools → Serial Monitor** at **115200 baud** to verify startup output.

> You may need to press the **BOOT** button on the LILYGO board while the
> IDE is trying to connect if upload fails.

---

## Matching LoRa configuration

The sender and receiver **must share identical LoRa radio parameters** or
packets will not be decoded. Both sketches are pre-configured with:

| Parameter         | Value      |
|-------------------|------------|
| Frequency         | 915 MHz    |
| Spreading factor  | 7          |
| Signal bandwidth  | 125 kHz    |
| Coding rate       | 4/5        |
| TX power (sender) | 17 dBm     |

If you change any parameter in one sketch, change it in the other to match.

---

## Troubleshooting

| Symptom                                 | Check                                                                 |
|-----------------------------------------|-----------------------------------------------------------------------|
| Upload fails                            | Hold BOOT button during upload; try a lower upload speed              |
| OLED shows nothing                      | Verify 3.3 V and GND; confirm I²C address is 0x3C                    |
| OLED shows "BMI160 not found"           | Check VIN, GND, SDA (GPIO 21), SCL (GPIO 22); confirm 0x69           |
| OLED shows "LoRa failed"               | Attach LoRa antenna; confirm correct frequency band                   |
| Receiver never receives packets         | Confirm both units use the same LoRa band (915E6 or 868E6)           |
| BMI160 not detected                     | Verify SA0 is unconnected (address 0x69); check I²C wiring            |
| No buzzer sound                         | Confirm GPIO 12 → S and GPIO 13 → − ; + pin unconnected               |
| LED wrong color or always off           | Confirm common anode to 3.3 V; check individual 220–330 Ω resistors   |
| Library not found compile error         | Install all four required libraries via Library Manager               |
| 3.3 V rail not powered                  | Confirm LILYGO 3.3 V pin is connected to the breadboard rail          |
| Cancel button not responding (sender)   | Confirm GPIO 2 → button → GND; no external pull-up needed             |
| Acknowledge button not responding (RX)  | Confirm GPIO 2 → button → GND; no external pull-up needed             |
| Battery voltage shows 0.00 V           | Confirm LiPo is connected to the T3 V1.6.1 battery connector; GPIO 35 is ADC-only |
| Battery voltage reads incorrectly       | Calibrate `BATTERY_VOLTAGE_DIVIDER` in sender firmware against a multimeter |

---

## Verifying operation

After both boards are running:

1. The **sender** OLED should show `SENDER` with `FallRisk: 0%` at rest, plus
   battery voltage and power source.
2. The **receiver** OLED should show `RECEIVER / Status: LISTENING`.
3. Perform a controlled fall motion — after 1.5 s of sustained high-confidence
   windows the sender LED turns red and the OLED shows a **5-second countdown**.
   - Press the **sender GPIO 2 button** within 5 seconds to cancel — no LoRa
     packet is sent and the OLED shows `CANCELLED`.
   - Let the countdown expire — the `FALL_ALERT` packet is transmitted, the
     sender buzzer sounds, and the OLED shows `FALL ALERT SENT`.
4. The **receiver** responds with a red LED, repeating buzzer, and OLED alert
   showing the alert number, confidence score, RSSI, and SNR. The alarm
   continues until the **receiver GPIO 2 button** is pressed.
5. After acknowledging, the receiver OLED returns to `LISTENING` and the LED
   returns to green.
