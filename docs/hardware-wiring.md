# ARMOR Hardware Wiring Guide

This document describes all physical connections required to build the ARMOR
sender (wearable) and receiver (base station) prototype on a breadboard.

---

## Boards

Both the sender and receiver use the **LILYGO T3 V1.6.1** — an ESP32
development board with an on-board SX1276 LoRa radio, a built-in 128×64
SSD1306 OLED display, and a Micro-USB connector.

---

## Internal LoRa GPIO pins — reserved on both boards

The SX1276 LoRa radio is connected internally on the T3 V1.6.1 PCB via the
following GPIO pins.  **These pins must not be used for any external device.**

| Signal   | GPIO |
|----------|------|
| SCK      | 5    |
| MISO     | 19   |
| MOSI     | 27   |
| CS / NSS | 18   |
| RESET    | 23   |
| DIO0     | 26   |

---

## Built-in OLED — both boards

The SSD1306 OLED panel is connected internally on both boards and shares the
ESP32's default I²C bus.

| Signal | GPIO | I²C address |
|--------|------|-------------|
| SDA    | 21   | 0x3C        |
| SCL    | 22   | 0x3C        |

The OLED I²C address is **0x3C**.

---

## Sender — DFRobot Gravity BMI160 (SEN0250)

The BMI160 six-axis IMU is wired to the **sender board only** over I²C.
It shares the I²C bus with the built-in OLED.

| BMI160 breakout pin | Connect to               |
|---------------------|--------------------------|
| VIN                 | Shared 3.3 V breadboard rail |
| GND                 | Shared GND breadboard rail   |
| SDA                 | GPIO 21                  |
| SCL                 | GPIO 22                  |
| 3V3                 | Unconnected              |
| SA0                 | Unconnected              |
| CS                  | Unconnected              |
| SDX                 | Unconnected              |
| SCX                 | Unconnected              |
| INT1                | Unconnected              |
| INT2                | Unconnected              |
| OCS                 | Unconnected              |

> **I²C address:** With SA0 left unconnected the BMI160 defaults to
> **0x69**.  The documented working firmware address is `0x69`.

> The BMI160 shares the I²C bus with the OLED.  Both devices are on
> GPIO 21 (SDA) and GPIO 22 (SCL).

---

## Common-anode RGB LED — sender and receiver

Both boards use an external common-anode RGB LED.  Each board requires one
LED wired as described below.

A common-anode LED has its shared positive leg connected to 3.3 V.  Each
color cathode is driven by a GPIO pin.  Because the anode is at a higher
voltage, pulling a GPIO **LOW turns the color ON**; setting it **HIGH turns
it OFF**.

**Each color channel must have its own current-limiting resistor.**

| LED leg                | Connect to                                      |
|------------------------|-------------------------------------------------|
| Common anode (long leg) | 3.3 V                                          |
| Red cathode            | GPIO 14 → 220–330 Ω resistor → GPIO 14         |
| Green cathode          | GPIO 25 → 220–330 Ω resistor → GPIO 25         |
| Blue cathode           | GPIO 4  → 220–330 Ω resistor → GPIO 4          |

> Use a separate 220–330 ohm resistor for each color cathode.  A single
> shared resistor will cause incorrect brightness and color mixing.

---

## KY-006 passive buzzer — sender and receiver

Both boards use a KY-006 passive buzzer module.

**Sender** uses a differential two-GPIO drive (both pins toggled opposite):

| KY-006 pin | Connect to |
|------------|------------|
| S (signal) | GPIO 12    |
| − (minus)  | GPIO 13    |
| Middle +   | Unconnected |

> GPIO 12 and GPIO 13 are always driven to opposite states so the full 3.3 V
> supply swing appears across the piezo, improving loudness at low voltage.
> **Neither buzzer terminal goes directly to GND.**

**Receiver** uses single-pin hardware PWM (LEDC) on GPIO 12; GPIO 13 is held
permanently LOW as the piezo return path:

| KY-006 pin | Connect to              |
|------------|-------------------------|
| S (signal) | GPIO 12 (LEDC PWM output) |
| − (minus)  | GPIO 13 (held LOW)      |
| Middle +   | Unconnected             |

> The receiver firmware generates a clean hardware square wave via the ESP32
> LEDC peripheral rather than bit-banging, allowing the alarm to repeat
> without blocking the LoRa receive or button-poll loop.
> **Neither buzzer terminal goes directly to GND.**

---

## Cancel / acknowledge buttons

Each board has one tactile button wired between a GPIO pin and GND.  The
firmware configures each pin as `INPUT_PULLUP`; the GPIO reads HIGH at rest
and LOW when the button is pressed.

| Board    | GPIO | Function                                                         |
|----------|------|------------------------------------------------------------------|
| Sender   | 2    | Cancel / "I'm OK" — suppresses the alert during the 5-second countdown |
| Receiver | 2    | Acknowledge — silences the repeating alarm after a FALL_ALERT is received |

**Wiring (same for both boards):**

```
GPIO 2 ──── normally-open pushbutton ──── GND
```

> No external pull-up resistor is needed; the ESP32 internal pull-up is
> enabled in firmware with `INPUT_PULLUP`.

---

## Battery monitoring (sender only)

The T3 V1.6.1 has an onboard resistor voltage divider that connects the
LiPo cell to **GPIO 35** (ADC1).  The sender firmware reads this pin to
estimate battery voltage and state of charge.

| Signal              | GPIO |
|---------------------|------|
| Battery ADC input   | 35   |

> GPIO 35 is input-only on the ESP32 — do not drive it as an output.
> The divider factor is initially estimated at 2.0 in firmware; calibrate
> against a multimeter for accuracy.

---

## Power rails

| Source                          | Breadboard rail               |
|---------------------------------|-------------------------------|
| LILYGO T3 3.3 V pin             | Positive (3.3 V) rail         |
| LILYGO T3 GND pin               | Negative (GND) rail           |
| BMI160 VIN                      | Positive (3.3 V) rail         |
| RGB LED common anode (long leg) | Positive (3.3 V) rail         |

> Both boards should have their LoRa antennas attached **before** any
> firmware is uploaded or power is applied.  Operating the SX1276 without
> an antenna may damage the radio output stage.

---

## Wiring summary diagram (text)

```
LILYGO T3 V1.6.1 (sender or receiver)
│
├── 3.3V ──────────────────────────────── Breadboard 3.3V rail
│                                           ├── BMI160 VIN (sender only)
│                                           └── RGB LED common anode
│
├── GND ───────────────────────────────── Breadboard GND rail
│                                           └── BMI160 GND (sender only)
│
├── GPIO 21 (SDA) ──────────────────────── BMI160 SDA (sender only)
│                                           └── (shared with built-in OLED)
│
├── GPIO 22 (SCL) ──────────────────────── BMI160 SCL (sender only)
│                                           └── (shared with built-in OLED)
│
├── GPIO 14 ── 220–330 Ω ─────────────── RGB LED red cathode
├── GPIO 25 ── 220–330 Ω ─────────────── RGB LED green cathode
├── GPIO 4  ── 220–330 Ω ─────────────── RGB LED blue cathode
│
├── GPIO 12 ─────────────────────────── KY-006 S pin
└── GPIO 13 ─────────────────────────── KY-006 − pin
```

---

## Internal LoRa pins (shown for reference — do not rewire)

```
LILYGO T3 V1.6.1 (internal PCB connections — do not add external wiring)
│
├── GPIO  5  → SX1276 SCK
├── GPIO 19  → SX1276 MISO
├── GPIO 27  → SX1276 MOSI
├── GPIO 18  → SX1276 CS/NSS
├── GPIO 23  → SX1276 RESET
└── GPIO 26  → SX1276 DIO0
```
