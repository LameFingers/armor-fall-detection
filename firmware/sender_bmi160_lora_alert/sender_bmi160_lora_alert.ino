/*
 * ============================================================
 * ARMOR — Edge-AI Wearable Fall-Detection & LoRa Emergency Alert
 * Firmware role: SENDER (wearable unit)
 * Board: LILYGO T3 V1.6.1 (ESP32 + SX1276 + SSD1306 OLED)
 *
 * Features:
 *   - BMI160 6-axis IMU feeding a TinyML continuous rolling buffer.
 *   - Edge Impulse binary classification (fall_like vs non_fall).
 *   - Five-second cancellation period after a fall_like event.
 *   - GPIO 2 button cancels the alert before LoRa transmission.
 *   - LoRa FALL_ALERT transmission and KY-006 buzzer after timeout.
 *   - Estimated LiPo voltage and percentage on the OLED.
 *
 * Alert flow:
 *   1. TinyML fall_like confidence exceeds FALL_LIKE_THRESHOLD for
 *      REQUIRED_CONSECUTIVE_FALLS inference windows and cooldown has elapsed.
 *   2. LED turns red; OLED shows a 5-second countdown.
 *   3. If the wearer presses GPIO 2 within 5 seconds: alert is cancelled,
 *      no LoRa packet is sent, LED returns green.
 *   4. If the countdown expires without a button press: alertNumber is
 *      incremented, FALL_ALERT packet is transmitted over LoRa, buzzer
 *      sounds, then LED returns green.
 *
 * Button wiring:
 *   GPIO 2 ---- pushbutton ---- GND
 *   Uses INPUT_PULLUP: unpressed = HIGH; pressed = LOW.
 *
 * Battery:
 *   PKCELL LP503562: 3.7 V nominal LiPo, 4.2 V full.
 *   T3 V1.6.1 battery-monitor ADC input: GPIO 35.
 *
 * This is a prototype, not a medical/safety-certified device.
 * ============================================================
 */

// Edge Impulse TinyML library (name matches your exported Arduino project).
#include <armor-fall-detection-binary-v1_inferencing.h>

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
// VIN → 3.3V rail, GND → GND rail.
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
// Both pins are driven to opposite logic levels so the full 3.3 V swing
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
// =================================================
#define BATTERY_ADC_PIN  35

const float BATTERY_VOLTAGE_DIVIDER = 2.0;
// How often (ms) the battery voltage is re-sampled.
const unsigned long BATTERY_UPDATE_MS = 5000;

// =================================================
// TINYML MOTION DETECTION SETTINGS
// =================================================

// Minimum fall_like confidence score (0–1) to count a window as fall-like.
const float FALL_LIKE_THRESHOLD = 0.8f;

// Number of consecutive fall-like inference windows required before an
// alert is triggered. Reduces false positives from isolated sharp motions.
const uint8_t REQUIRED_CONSECUTIVE_FALLS = 3;

// How often (ms) the BMI160 is sampled and the rolling buffer is updated.
const unsigned long SENSOR_UPDATE_MS = 20;

// Minimum time (ms) between successive classifier runs.
const unsigned long INFERENCE_INTERVAL_MS = 500;

// Duration (ms) the alert/cancelled screen is shown after the sequence ends.
const unsigned long ALERT_DISPLAY_MS = 2000;

// Minimum time (ms) between successive alert triggers.
const unsigned long ALERT_COOLDOWN_MS = 5000;

// How long (ms) the wearer has to press the cancel button before the
// LoRa packet is sent. Shown as a countdown on the OLED.
const unsigned long CANCEL_COUNTDOWN_MS = 5000;

// Minimum stable button duration (ms) required to accept a press.
const unsigned long BUTTON_DEBOUNCE_MS = 50;

// =================================================
// Runtime state
// =================================================

unsigned long lastSensorUpdate  = 0;
unsigned long lastAlertTime     = 0;
unsigned long lastBatteryUpdate = 0;

// Sequential alert number included in every transmitted FALL_ALERT packet.
// Only incremented when the countdown expires and the packet is actually sent.
uint32_t alertNumber = 0;

float batteryVoltage    = 0.0;
uint8_t batteryPercent  = 0;
// True when USB serial is active (used as a USB-power presence estimate).
bool usbPowerPresent = false;

// Count of consecutive inference windows that scored above the threshold.
uint8_t consecutiveFallWindows = 0;

// =================================================
// TinyML rolling buffer state
// =================================================

float featureBuffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = {0};
unsigned long lastInferenceTime = 0;
bool bufferFilled = false;
int sampleCount   = 0;

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
  // With ADC_11db attenuation, ESP32 ADC maps roughly 0–3.3 V into 0–4095.
  float adcVoltage = (adcAverage / 4095.0) * 3.3;
  return adcVoltage * BATTERY_VOLTAGE_DIVIDER;
}

// Approximate LiPo state of charge derived from open-circuit voltage.
// Readings may dip briefly during LoRa TX or buzzer operation due to
// supply current draw.
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
// intervals. The batteryVoltage == 0.0 guard lets setup() force an immediate
// first reading before the first loop() cycle.
void updateBatteryStatus() {
  if (batteryVoltage > 0.0 &&
      millis() - lastBatteryUpdate < BATTERY_UPDATE_MS) {
    return;
  }

  lastBatteryUpdate = millis();
  batteryVoltage  = readBatteryVoltage();
  batteryPercent  = batteryVoltageToPercent(batteryVoltage);
  // Serial is truthy while USB serial is active; used as a proxy for USB power.
  usbPowerPresent = Serial;
}

// =================================================
// OLED functions
// =================================================

// Normal monitoring screen: live fall-like confidence, threshold, battery,
// and power source. Updated every INFERENCE_INTERVAL_MS.
void showMonitoring(float confidence) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("SENDER");

  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print("FallRisk:");
  display.print(confidence * 100, 0);
  display.print("% T:");
  display.print(FALL_LIKE_THRESHOLD * 100, 0);
  display.println("%");

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
void showAlert(float confidence) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("MOTION!");

  display.setTextSize(1);
  display.setCursor(0, 23);
  display.println("FALL ALERT SENT");
  display.print("Conf: ");
  display.print(confidence * 100, 0);
  display.println("%");
  display.print("Alert #");
  display.println(alertNumber);

  display.display();
}

// Cancel countdown screen: updated each second during the 5-second window.
void showCancelCountdown(float confidence, uint8_t secondsRemaining) {
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
void showAlertCancelled(float confidence) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("CANCELLED");

  display.setTextSize(1);
  display.setCursor(0, 27);
  display.println("Alert was not sent");
  display.print("Conf: ");
  display.print(confidence * 100, 0);
  display.println("%");

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
// so the full 3.3 V swing appears across the passive piezo element.
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
// Packet format: "FALL_ALERT,<alertNumber>,CONF=<confidence>"
// Example:       "FALL_ALERT,3,CONF=0.94"
// Only transmitted after the cancel countdown expires without a button press.
// =================================================
void sendFallAlert(float confidence) {
  String message = "FALL_ALERT," + String(alertNumber) +
                   ",CONF=" + String(confidence, 2);

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
bool waitForCancelButton(float confidence) {
  unsigned long countdownStart   = millis();
  bool lastRawButtonReading      = digitalRead(CANCEL_BUTTON_PIN);
  bool stableButtonState         = lastRawButtonReading;
  unsigned long lastDebounceTime = millis();

  // Track seconds shown to avoid redundant OLED refreshes.
  uint8_t previouslyShownSeconds = 255;

  while (millis() - countdownStart < CANCEL_COUNTDOWN_MS) {
    unsigned long elapsedMs   = millis() - countdownStart;
    unsigned long remainingMs = CANCEL_COUNTDOWN_MS - elapsedMs;
    // Ceiling division: show "1 sec" until the very last millisecond.
    uint8_t secondsRemaining  = (remainingMs + 999) / 1000;

    if (secondsRemaining != previouslyShownSeconds) {
      showCancelCountdown(confidence, secondsRemaining);
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

      // Wait for release to avoid immediately re-detecting a held button.
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
// Called when consecutiveFallWindows reaches REQUIRED_CONSECUTIVE_FALLS
// and the cooldown has elapsed.
//
// Sequence:
//   1. Start cooldown timer (prevents re-triggering during countdown).
//   2. Turn LED red.
//   3. Run 5-second cancel countdown.
//      → Cancelled: show CANCELLED screen, return to green/monitoring.
//      → Expired:   increment alertNumber, transmit FALL_ALERT, sound buzzer,
//                   show alert screen, return to green/monitoring.
// In both paths the rolling buffer is cleared so stale data does not
// immediately re-trigger an alert after the cooldown period.
// =================================================
void triggerAlert(float confidence) {
  // Start cooldown even if cancelled — prevents repeated countdowns from
  // one sustained high-confidence sequence.
  lastAlertTime = millis();
  ledRed();

  if (waitForCancelButton(confidence)) {
    showAlertCancelled(confidence);
    delay(ALERT_DISPLAY_MS);
    ledGreen();
    showMonitoring(confidence);

    memset(featureBuffer, 0, sizeof(featureBuffer));
    sampleCount   = 0;
    bufferFilled  = false;
    return;
  }

  alertNumber++;
  showAlert(confidence);
  sendFallAlert(confidence);
  buzzerAlarm();

  delay(ALERT_DISPLAY_MS);
  ledGreen();
  showMonitoring(confidence);

  memset(featureBuffer, 0, sizeof(featureBuffer));
  sampleCount   = 0;
  bufferFilled  = false;
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
    while (true) delay(1000);
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
    while (true) delay(1000);
  }

  Serial.println("BMI160 initialized at I2C address 0x69.");

  // Configure SPI and initialize the SX1276 LoRa radio.
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa failed.");
    showError("LoRa failed", "Check antenna/band.");
    while (true) delay(1000);
  }

  // Radio parameters — must match receiver exactly.
  LoRa.setTxPower(17);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  // Take an initial battery reading so the OLED shows a value immediately.
  updateBatteryStatus();

  Serial.println("Sender is ready.");
  showMonitoring(0.0);
}

// =================================================
// loop() — TinyML continuous inference
//
// Every SENSOR_UPDATE_MS:
//   1. Read BMI160 accelerometer and gyroscope.
//   2. Shift the rolling feature buffer and append the new 6-axis sample.
//   3. After SENSOR_UPDATE_MS * 100 warm-up samples, run the classifier
//      every INFERENCE_INTERVAL_MS.
//   4. If fall_like confidence exceeds the threshold for
//      REQUIRED_CONSECUTIVE_FALLS consecutive windows, trigger the alert.
// =================================================
void loop() {
  if (millis() - lastSensorUpdate < SENSOR_UPDATE_MS) {
    return;
  }
  lastSensorUpdate = millis();

  // Six-element array: [0..2] = gyro X/Y/Z, [3..5] = accel X/Y/Z.
  int16_t sensorData[6] = {0};

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

  // Shift the rolling buffer left by one 6-axis sample and append the new one.
  for (int i = 0; i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 6; i++) {
    featureBuffer[i] = featureBuffer[i + 6];
  }

  int lastIndex = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 6;
  featureBuffer[lastIndex + 0] = ax;
  featureBuffer[lastIndex + 1] = ay;
  featureBuffer[lastIndex + 2] = az;
  featureBuffer[lastIndex + 3] = gx;
  featureBuffer[lastIndex + 4] = gy;
  featureBuffer[lastIndex + 5] = gz;

  // Warm-up: discard the first 100 samples so the buffer is fully populated
  // before any inference is attempted.
  if (sampleCount < 100) {
    sampleCount++;
    return;
  }

  bufferFilled = true;

  if (millis() - lastInferenceTime < INFERENCE_INTERVAL_MS) {
    return;
  }
  lastInferenceTime = millis();

  signal_t features_signal;
  int err = numpy::signal_from_buffer(
      featureBuffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &features_signal);

  ei_impulse_result_t result = {0};
  err = run_classifier(&features_signal, &result, false);

  if (err != EI_IMPULSE_OK) {
    Serial.print("ERR: Failed to run classifier, code: ");
    Serial.println(err);
    return;
  }

  float fallScore = 0.0;
  for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (strcmp(result.classification[i].label, "fall_like") == 0) {
      fallScore = result.classification[i].value;
    }
  }

  Serial.print("Fall: ");
  Serial.print(fallScore, 2);

  updateBatteryStatus();
  showMonitoring(fallScore);

  bool cooldownFinished = (millis() - lastAlertTime >= ALERT_COOLDOWN_MS);

  if (fallScore >= FALL_LIKE_THRESHOLD) {
    consecutiveFallWindows++;
  } else {
    consecutiveFallWindows = 0;
  }

  Serial.print(" FallWindows: ");
  Serial.println(consecutiveFallWindows);

  if (cooldownFinished && consecutiveFallWindows >= REQUIRED_CONSECUTIVE_FALLS) {
    Serial.println(">>> CONFIRMED FALL-LIKE EVENT");
    consecutiveFallWindows = 0;
    triggerAlert(fallScore);
  }
}
