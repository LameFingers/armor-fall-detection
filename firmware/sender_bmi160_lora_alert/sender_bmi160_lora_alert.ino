/*
 * ============================================================
 * ARMOR — Edge-AI Wearable Fall-Detection & LoRa Emergency Alert
 * Firmware role: SENDER (wearable unit)
 * Board: LILYGO T3 V1.6.1 (ESP32 + SX1276 + SSD1306 OLED)
 *
 * Features:
 *   - BMI160 acceleration monitoring and threshold-based motion alerts.
 *   - Five-second cancellation period after a motion event.
 *   - GPIO 2 button cancels the alert before LoRa transmission.
 *   - LoRa FALL_ALERT transmission and KY-006 buzzer after timeout.
 *   - Estimated LiPo voltage and percentage on the OLED.
 *
 * Alert flow:
 *   1. BMI160 magnitude exceeds MOTION_THRESHOLD_G and cooldown has elapsed.
 *   2. LED turns red; OLED shows a 5-second countdown.
 *   3. If the wearer presses GPIO 2 within 5 seconds: alert is cancelled,
 *      no LoRa packet is sent, LED returns green.
 *   4. If the countdown expires without a button press: alertNumber is
 *      incremented, FALL_ALERT packet is transmitted over LoRa, buzzer
 *      sounds, then LED returns green.
 *
 * This design is a baseline threshold detector, NOT a validated fall
 * detector. A single acceleration threshold may cause false positives
 * (e.g. sharp arm movements) and may miss low-acceleration falls.
 *
 * Button wiring:
 *   GPIO 2 ---- pushbutton ---- GND
 *   Uses INPUT_PULLUP: unpressed = HIGH; pressed = LOW.
 *
 * Battery:
 *   PKCELL LP503562: 3.7 V nominal LiPo, 4.2 V full.
 *   T3 V1.6.1 battery-monitor ADC input: GPIO 35.
 *
 * This is a baseline prototype, not a medical/safety-certified device.
 * ============================================================
 */

// SPI driver — required for the SX1276 LoRa module via hardware SPI.
#include <SPI.h>
// LoRa packet radio driver by Sandeep Mistry.
#include <LoRa.h>
// I2C driver — used by both the OLED and the BMI160.
#include <Wire.h>
// Adafruit GFX graphics primitives — required by the SSD1306 driver.
#include <Adafruit_GFX.h>
// Adafruit SSD1306 OLED driver for the built-in 128×64 display.
#include <Adafruit_SSD1306.h>
// DFRobot driver for the Gravity BMI160 (SEN0250) 6-axis IMU.
#include <DFRobot_BMI160.h>
// Standard C math — used for sqrt() in magnitude calculation.
#include <math.h>

// =================================================
// LILYGO T3 V1.6.1 INTERNAL LORA CONNECTIONS
//
// Hardwired on the PCB. Do not connect any external peripheral to these GPIOs.
// =================================================
#define LORA_SCK   5
#define LORA_MISO  19
#define LORA_MOSI  27
#define LORA_SS    18
#define LORA_RST   23
#define LORA_DIO0  26

// Operating frequency — must match the receiver exactly.
// 915E6 = 915 MHz (Australia, North America). Change to 868E6 for EU modules.
#define LORA_BAND  915E6

// =================================================
// BUILT-IN OLED: I2C
//
// SSD1306 128×64 connected internally on the T3 V1.6.1.
// GPIO21 = SDA, GPIO22 = SCL, I2C address = 0x3C.
// Shared bus with the BMI160.
// =================================================
#define OLED_SDA   21
#define OLED_SCL   22
#define OLED_ADDR  0x3C
#define SCREEN_W   128
#define SCREEN_H   64

// -1 reset argument: no dedicated hardware reset pin; uses software reset.
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// =================================================
// DFRobot Gravity BMI160: I2C
//
// Shares the I2C bus with the OLED (GPIO21 SDA, GPIO22 SCL).
// SA0 left unconnected → default address 0x69.
// VIN → 3.3V rail, GND → GND rail. All other breakout pins unconnected.
// =================================================
#define BMI160_ADDR  0x69
DFRobot_BMI160 bmi160;

// =================================================
// EXTERNAL COMMON-ANODE RGB LED
//
// Common anode (long leg) → 3.3V.
// Each cathode connects through a 220–330 Ω resistor to its GPIO.
// LOW means ON; HIGH means OFF.
// =================================================
#define LED_RED    14
#define LED_GREEN  25
#define LED_BLUE   4

// =================================================
// KY-006 PASSIVE PIEZO, DIFFERENTIAL TWO-PIN DRIVE
//
// Wiring:
//   KY-006 S / piezo terminal → GPIO 12
//   KY-006 - / piezo terminal → GPIO 13
//   KY-006 + pin → unconnected
//
// Both pins are driven to opposite logic levels so the full 3.3V swing
// appears across the piezo, improving loudness at low supply voltage.
// Neither terminal connects directly to GND.
// =================================================
#define BUZZER_A   12
#define BUZZER_B   13

// =================================================
// FALL-ALERT CANCEL BUTTON
//
// Wiring: GPIO 2 ---- normally-open pushbutton ---- GND
// Configured INPUT_PULLUP: unpressed = HIGH, pressed = LOW.
// Pressing within CANCEL_COUNTDOWN_MS cancels the alert before any
// LoRa packet is sent.
// =================================================
#define CANCEL_BUTTON_PIN  2

// =================================================
// BATTERY MONITORING
//
// GPIO 35 is the T3 V1.6.1 battery voltage ADC input.
// The board has an onboard resistor divider; the divider factor is
// initially estimated as 2.0. Calibrate against a multimeter:
//   - Displayed voltage too low → increase BATTERY_VOLTAGE_DIVIDER.
//   - Displayed voltage too high → decrease BATTERY_VOLTAGE_DIVIDER.
// Voltage readings may dip briefly during LoRa TX or buzzer operation
// due to supply current draw.
// =================================================
#define BATTERY_ADC_PIN  35

const float BATTERY_VOLTAGE_DIVIDER = 2.0;
// How often (ms) the battery voltage is re-sampled.
const unsigned long BATTERY_UPDATE_MS = 5000;

// =================================================
// MOTION DETECTION SETTINGS
// =================================================

// Acceleration magnitude threshold. At rest the BMI160 reads ~1 g because
// gravity is always included. 1.2 g is a conservative starting point;
// calibrate upward using Serial Monitor readings to reduce false positives.
const float MOTION_THRESHOLD_G = 1.2;

// Duration (ms) the alert LED/display stay shown after transmission.
const unsigned long ALERT_DISPLAY_MS = 2000;

// Minimum time (ms) between successive alert triggers.
// Prevents repeated alerts from a single sustained high-magnitude event.
const unsigned long ALERT_COOLDOWN_MS = 5000;

// How often (ms) the sensor is read and the OLED is refreshed.
const unsigned long SENSOR_UPDATE_MS = 500;

// How long (ms) the wearer has to press the cancel button before the
// LoRa packet is sent. Shown as a countdown on the OLED.
const unsigned long CANCEL_COUNTDOWN_MS = 5000;

// Minimum stable button duration (ms) required to accept a press.
const unsigned long BUTTON_DEBOUNCE_MS = 50;

// =================================================
// Runtime state
// =================================================

// Tracks the last time the sensor was polled.
unsigned long lastSensorUpdate = 0;
// Tracks the last time an alert fired; used for cooldown enforcement.
unsigned long lastAlertTime = 0;
// Tracks the last time the battery voltage was sampled.
unsigned long lastBatteryUpdate = 0;

// Sequential alert number included in every transmitted FALL_ALERT packet.
// Only incremented when the countdown expires and the packet is actually sent.
uint32_t alertNumber = 0;

// Most recent battery reading; updated by updateBatteryStatus().
float batteryVoltage = 0.0;
uint8_t batteryPercent = 0;
// True when USB serial is active (used as a USB-power presence estimate).
bool usbPowerPresent = false;

// =================================================
// LED helpers
// =================================================

// Normal monitoring state: green LED indicates the device is running.
void ledGreen() {
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, HIGH);
}

// Alert / countdown state: red LED indicates a pending or sent alert.
void ledRed() {
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
}

// =================================================
// Battery functions
// =================================================

// Reads BATTERY_ADC_PIN 16 times, averages, and converts to volts using
// the 11 dB attenuation full-scale (3.3 V) and the board voltage divider.
float readBatteryVoltage() {
  const uint8_t SAMPLE_COUNT = 16;
  uint32_t total = 0;

  for (uint8_t i = 0; i < SAMPLE_COUNT; i++) {
    total += analogRead(BATTERY_ADC_PIN);
    delay(2);
  }

  float adcAverage = total / (float)SAMPLE_COUNT;

  // With ADC_11db attenuation, ESP32 ADC maps roughly 0–3.3 V
  // into the 0–4095 raw conversion range.
  float adcVoltage = (adcAverage / 4095.0) * 3.3;

  return adcVoltage * BATTERY_VOLTAGE_DIVIDER;
}

// Approximate LiPo state of charge derived from voltage.
// Load from LoRa transmission/buzzer can make this temporarily read lower.
uint8_t batteryVoltageToPercent(float voltage) {
  if (voltage >= 4.20) return 100;
  if (voltage >= 4.15) return 95;
  if (voltage >= 4.10) return 90;
  if (voltage >= 4.05) return 85;
  if (voltage >= 4.00) return 80;
  if (voltage >= 3.95) return 75;
  if (voltage >= 3.90) return 70;
  if (voltage >= 3.85) return 65;
  if (voltage >= 3.80) return 60;
  if (voltage >= 3.75) return 50;
  if (voltage >= 3.70) return 40;
  if (voltage >= 3.65) return 30;
  if (voltage >= 3.60) return 20;
  if (voltage >= 3.50) return 10;
  if (voltage >= 3.30) return 5;
  return 0;
}

// Re-samples battery voltage and USB-presence flag at BATTERY_UPDATE_MS
// intervals. Skips the first call guard (batteryVoltage == 0.0) so that
// an initial reading is taken at setup() before the first loop() cycle.
void updateBatteryStatus() {
  if (batteryVoltage > 0.0 &&
      millis() - lastBatteryUpdate < BATTERY_UPDATE_MS) {
    return;
  }

  lastBatteryUpdate = millis();
  batteryVoltage = readBatteryVoltage();
  batteryPercent = batteryVoltageToPercent(batteryVoltage);

  /*
   * USB-present estimate:
   * On many ESP32 Arduino configurations, Serial is true while USB serial
   * is connected. This does NOT directly measure charging current.
   *
   * Therefore OLED says "USB POWER" rather than "CHARGING."
   */
  usbPowerPresent = Serial;

  Serial.print("Battery: ");
  Serial.print(batteryVoltage, 2);
  Serial.print(" V, ");
  Serial.print(batteryPercent);
  Serial.println("%");
}

// =================================================
// OLED functions
// =================================================

// Normal monitoring screen: live acceleration, threshold, battery, and
// power source. Updated every SENSOR_UPDATE_MS.
void showMonitoring(float accelerationMagnitude) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("SENDER");

  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print("Acc: ");
  display.print(accelerationMagnitude, 2);
  display.print("g  T:");
  display.print(MOTION_THRESHOLD_G, 1);
  display.println("g");

  display.setCursor(0, 33);
  display.print("Bat: ");
  display.print(batteryPercent);
  display.print("% ");
  display.print(batteryVoltage, 2);
  display.println("V");

  display.setCursor(0, 48);
  if (usbPowerPresent) {
    display.println("Power: USB");
  } else {
    display.println("Power: BATTERY");
  }

  display.display();
}

// Post-transmission screen: shown for ALERT_DISPLAY_MS after the LoRa
// packet has been sent and the buzzer has sounded.
void showAlert(float accelerationMagnitude) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("MOTION!");

  display.setTextSize(1);
  display.setCursor(0, 23);
  display.println("FALL ALERT SENT");
  display.print("Impact: ");
  display.print(accelerationMagnitude, 2);
  display.println(" g");
  display.print("Alert #");
  display.println(alertNumber);
  display.display();
}

// Cancel countdown screen: updated each second during the 5-second window.
// Shows the magnitude that triggered the alert and the seconds remaining.
void showCancelCountdown(float accelerationMagnitude,
                         uint8_t secondsRemaining) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("MOTION!");

  display.setTextSize(1);
  display.setCursor(0, 22);
  display.println("Press button");
  display.println("to cancel alert");

  display.setTextSize(2);
  display.setCursor(0, 43);
  display.print(secondsRemaining);
  display.println(" sec");

  display.display();
}

// Confirmation screen: shown briefly when the wearer cancels in time.
void showAlertCancelled(float accelerationMagnitude) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("CANCELLED");

  display.setTextSize(1);
  display.setCursor(0, 27);
  display.println("Alert was not sent");
  display.print("Impact: ");
  display.print(accelerationMagnitude, 2);
  display.println(" g");

  display.display();
}

// Error screen: shown when setup() fails to initialize a peripheral.
void showError(const char *message1, const char *message2) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("ERROR");

  display.setTextSize(1);
  display.setCursor(0, 28);
  display.println(message1);
  display.println(message2);

  display.display();
}

// =================================================
// Buzzer functions
//
// Differential drive: BUZZER_A and BUZZER_B are toggled to opposite levels
// so the full 3.3V swing appears across the passive piezo element.
// =================================================

// Generates a square wave of frequencyHz for durationMs using busy-wait.
// Called only after the cancel window has expired, so blocking here is
// acceptable — the FALL_ALERT packet is transmitted before this runs.
void differentialTone(uint16_t frequencyHz, uint16_t durationMs) {
  unsigned long halfPeriodUs = 500000UL / frequencyHz;
  unsigned long beginTime = millis();

  while (millis() - beginTime < durationMs) {
    digitalWrite(BUZZER_A, HIGH);
    digitalWrite(BUZZER_B, LOW);
    delayMicroseconds(halfPeriodUs);

    digitalWrite(BUZZER_A, LOW);
    digitalWrite(BUZZER_B, HIGH);
    delayMicroseconds(halfPeriodUs);
  }
}

// Alert tone: three sweeping pairs of tones near the piezo resonant region.
void buzzerAlarm() {
  for (int i = 0; i < 3; i++) {
    differentialTone(1900, 160);
    differentialTone(2300, 160);
  }

  // Both pins LOW: no voltage across piezo, buzzer silent.
  digitalWrite(BUZZER_A, LOW);
  digitalWrite(BUZZER_B, LOW);
}

// =================================================
// LoRa transmission
//
// Packet format: "FALL_ALERT,<alertNumber>,MAG=<magnitude>"
// Example:       "FALL_ALERT,3,MAG=1.87"
// Only transmitted after the cancel countdown expires without a button press.
// =================================================
void sendFallAlert(float accelerationMagnitude) {
  String message = "FALL_ALERT," + String(alertNumber) +
                   ",MAG=" + String(accelerationMagnitude, 2);

  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();

  Serial.print(">>> SENT: ");
  Serial.println(message);
}

// =================================================
// Cancellation countdown
//
// Blocks for up to CANCEL_COUNTDOWN_MS while polling the cancel button
// and updating the OLED countdown display each second.
// Returns true if the button was pressed (alert cancelled).
// Returns false if the countdown expired (proceed with transmission).
// =================================================
bool waitForCancelButton(float accelerationMagnitude) {
  unsigned long countdownStart = millis();

  bool lastRawButtonReading = digitalRead(CANCEL_BUTTON_PIN);
  bool stableButtonState = lastRawButtonReading;
  unsigned long lastDebounceTime = millis();

  // Track seconds shown to avoid redundant OLED refreshes.
  uint8_t previouslyShownSeconds = 255;

  while (millis() - countdownStart < CANCEL_COUNTDOWN_MS) {
    unsigned long elapsedMs = millis() - countdownStart;
    unsigned long remainingMs = CANCEL_COUNTDOWN_MS - elapsedMs;
    // Ceiling division: show "1 sec" until the very last millisecond.
    uint8_t secondsRemaining = (remainingMs + 999) / 1000;

    if (secondsRemaining != previouslyShownSeconds) {
      showCancelCountdown(accelerationMagnitude, secondsRemaining);
      previouslyShownSeconds = secondsRemaining;
    }

    bool rawButtonReading = digitalRead(CANCEL_BUTTON_PIN);

    if (rawButtonReading != lastRawButtonReading) {
      lastDebounceTime = millis();
    }

    if (millis() - lastDebounceTime >= BUTTON_DEBOUNCE_MS) {
      stableButtonState = rawButtonReading;
    }

    lastRawButtonReading = rawButtonReading;

    // INPUT_PULLUP means LOW is a pressed button.
    if (stableButtonState == LOW) {
      Serial.println(">>> Alert cancelled by GPIO2 button.");

      // Wait for release to avoid immediately detecting a held button
      // on the next event.
      while (digitalRead(CANCEL_BUTTON_PIN) == LOW) {
        delay(10);
      }

      return true;
    }

    delay(10);
  }

  return false;
}

// =================================================
// Alert sequence
//
// Called when magnitude >= MOTION_THRESHOLD_G and cooldown has elapsed.
// Sequence:
//   1. Start cooldown timer (prevents re-triggering during countdown).
//   2. Turn LED red.
//   3. Run 5-second cancel countdown.
//      → Cancelled: show CANCELLED screen, return to green/monitoring.
//      → Expired:   increment alertNumber, transmit FALL_ALERT, sound buzzer,
//                   show alert screen, return to green/monitoring.
// =================================================
void triggerAlert(float accelerationMagnitude) {
  // Start cooldown even if the wearer cancels the event. This avoids
  // repeated countdowns from one sustained acceleration reading.
  lastAlertTime = millis();

  ledRed();

  // No LoRa packet and no buzzer occur unless this times out.
  if (waitForCancelButton(accelerationMagnitude)) {
    showAlertCancelled(accelerationMagnitude);
    delay(ALERT_DISPLAY_MS);

    ledGreen();
    showMonitoring(accelerationMagnitude);
    return;
  }

  // Countdown ended without cancellation: send the emergency message.
  alertNumber++;

  showAlert(accelerationMagnitude);
  sendFallAlert(accelerationMagnitude);
  buzzerAlarm();

  delay(ALERT_DISPLAY_MS);

  ledGreen();
  showMonitoring(accelerationMagnitude);
}

// =================================================
// setup() — runs once at power-on or reset
//
// Initialization order:
//   1. GPIO (LED, buzzer, button).
//   2. ADC configuration for battery monitoring.
//   3. I2C bus → OLED → BMI160.
//   4. SPI bus → LoRa radio.
//
// Halts with an error screen if any peripheral fails so the fault
// can be identified before the device is deployed.
// =================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  // RGB LED — start green (monitoring / ready).
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  ledGreen();

  // Buzzer — both pins start LOW (piezo silent).
  pinMode(BUZZER_A, OUTPUT);
  pinMode(BUZZER_B, OUTPUT);
  digitalWrite(BUZZER_A, LOW);
  digitalWrite(BUZZER_B, LOW);

  // Cancel button — pulled HIGH internally; button pulls to GND.
  pinMode(CANCEL_BUTTON_PIN, INPUT_PULLUP);

  // Battery ADC — GPIO35 is ADC1 and works with 12-bit conversion.
  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);

  // OLED and BMI160 share this I2C bus.
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize the built-in OLED. Halts if not found.
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED failed.");

    while (true) {
      delay(1000);
    }
  }

  // Show startup message while BMI160 initializes.
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Starting BMI160...");
  display.display();

  // Initialize BMI160 at I2C address 0x69. Halts if not found.
  if (bmi160.I2cInit(BMI160_ADDR) != BMI160_OK) {
    Serial.println("BMI160 failed at address 0x69.");
    showError("BMI160 not found", "Check VIN/GND/SDA/SCL.");

    while (true) {
      delay(1000);
    }
  }

  Serial.println("BMI160 initialized at I2C address 0x69.");

  // Configure SPI and initialize the SX1276 LoRa radio.
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa failed.");
    showError("LoRa failed", "Check antenna/band.");

    while (true) {
      delay(1000);
    }
  }

  // Radio parameters — must match receiver exactly.
  LoRa.setTxPower(17);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  // Take an initial battery reading so the OLED shows a value immediately.
  updateBatteryStatus();

  Serial.println("Sender is ready.");
  Serial.println("Columns: GX GY GZ (raw) | AX AY AZ (g) | MAG (g)");

  showMonitoring(1.0);
}

// =================================================
// loop() — runs continuously after setup()
// =================================================
void loop() {
  // Rate-limit sensor reads to SENSOR_UPDATE_MS. Returns early if the
  // interval has not elapsed yet; triggerAlert() will block this guard
  // during the countdown window, which is the intended behavior.
  if (millis() - lastSensorUpdate < SENSOR_UPDATE_MS) {
    return;
  }

  lastSensorUpdate = millis();

  // Six-element array to receive gyro and accelerometer raw data.
  int16_t sensorData[6] = {0};

  // Skip this cycle on I2C read error.
  if (bmi160.getAccelGyroData(sensorData) != BMI160_OK) {
    Serial.println("BMI160 read error.");
    return;
  }

  // DFRobot library data order:
  //   sensorData[0..2] = Gyroscope X, Y, Z (raw counts)
  //   sensorData[3..5] = Accelerometer X, Y, Z (raw counts)
  float gx = sensorData[0];
  float gy = sensorData[1];
  float gz = sensorData[2];

  // BMI160 ±2 g range: 16,384 LSB/g.
  float ax = sensorData[3] / 16384.0;
  float ay = sensorData[4] / 16384.0;
  float az = sensorData[5] / 16384.0;

  // Total acceleration magnitude: sqrt(ax² + ay² + az²).
  // At rest this is ~1 g because gravity is always in the measurement.
  // Values significantly above 1 g indicate dynamic motion.
  float magnitudeG = sqrt(ax * ax + ay * ay + az * az);

  Serial.print("Gyro raw: X=");
  Serial.print(gx);
  Serial.print(" Y=");
  Serial.print(gy);
  Serial.print(" Z=");
  Serial.print(gz);

  Serial.print(" | Accel g: X=");
  Serial.print(ax, 3);
  Serial.print(" Y=");
  Serial.print(ay, 3);
  Serial.print(" Z=");
  Serial.print(az, 3);

  Serial.print(" | Magnitude=");
  Serial.print(magnitudeG, 3);
  Serial.println(" g");

  // Re-sample battery voltage on its own slower interval.
  updateBatteryStatus();
  showMonitoring(magnitudeG);

  // Suppress consecutive alerts from one sustained high-magnitude event.
  bool cooldownFinished =
      (millis() - lastAlertTime >= ALERT_COOLDOWN_MS);

  if (magnitudeG >= MOTION_THRESHOLD_G && cooldownFinished) {
    triggerAlert(magnitudeG);
  }
}
