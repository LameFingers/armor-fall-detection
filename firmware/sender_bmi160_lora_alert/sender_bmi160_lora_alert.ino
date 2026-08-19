/*
 * ============================================================
 * ARMOR — Edge-AI Wearable Fall-Detection & LoRa Emergency Alert
 * Firmware role: SENDER (wearable unit)
 * Board: LILYGO T3 V1.6.1 (ESP32 + built-in SX1276 LoRa + built-in SSD1306 OLED)
 *
 * What this firmware does:
 *   - Reads 6-axis motion data from a DFRobot Gravity BMI160 (I2C).
 *   - Calculates the total acceleration magnitude in g.
 *   - When magnitude exceeds MOTION_THRESHOLD_G the firmware triggers a
 *     "motion alert" — it is a baseline threshold alert, NOT a validated
 *     fall detector. A single acceleration threshold may cause false positives
 *     (e.g. from sharp arm movements) and may miss low-acceleration falls.
 *   - On alert: turns LED red, sounds the KY-006 buzzer, and broadcasts a
 *     FALL_ALERT LoRa packet so the receiver board can react.
 *   - During normal monitoring: LED stays green and the OLED shows live
 *     acceleration data.
 *
 * This firmware is a working baseline prototype. It has been physically tested
 * but is NOT medically certified or safety-certified in any way.
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
// The SX1276 radio is wired internally on the T3 V1.6.1 PCB.
// These GPIO pins are occupied by the radio hardware and must NOT
// be used for any external peripheral.
// =================================================
#define LORA_SCK   5
#define LORA_MISO  19
#define LORA_MOSI  27
#define LORA_SS    18
#define LORA_RST   23
#define LORA_DIO0  26

// Operating frequency for the SX1276. Both sender and receiver must use the
// same band. 915E6 = 915 MHz (Australia, North America). Change to 868E6 for
// EU 868 MHz hardware — but only if BOTH units are 868 MHz modules.
// Change this only if BOTH of your LoRa modules are a different band.
#define LORA_BAND  915E6

// =================================================
// BUILT-IN OLED: I2C
//
// The SSD1306 OLED panel on the T3 V1.6.1 is connected internally to
// the ESP32's default I2C bus.  The same bus also carries the BMI160.
// GPIO21 = SDA, GPIO22 = SCL, I2C address = 0x3C.
// =================================================
#define OLED_SDA   21
#define OLED_SCL   22
#define OLED_ADDR  0x3C
#define SCREEN_W   128
#define SCREEN_H   64

// Construct the display object. The -1 reset argument means no dedicated
// hardware reset pin — the OLED resets via software.
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// =================================================
// DFRobot Gravity BMI160: I2C
// SA0 left unconnected = 0x69 default
//
// The BMI160 breakout (DFRobot SEN0250) shares the I2C bus with the OLED.
// When SA0 is left unconnected the module defaults to I2C address 0x69.
// VIN -> 3.3V rail, GND -> GND rail, SDA -> GPIO21, SCL -> GPIO22.
// All other BMI160 breakout pins (3V3, SA0, CS, SDX, SCX, INT1, INT2,
// OCS) are left unconnected for this configuration.
// =================================================
#define BMI160_ADDR  0x69
DFRobot_BMI160 bmi160;

// =================================================
// EXTERNAL COMMON-ANODE RGB LED
// LOW means ON; HIGH means OFF
//
// A common-anode RGB LED has its shared anode (long leg) connected to 3.3V.
// Each color cathode is connected through a 220–330 ohm current-limiting
// resistor to the GPIO pin shown below.  Because the anode is at a higher
// potential, pulling a GPIO LOW forward-biases that color's diode and
// turns it ON; setting the GPIO HIGH turns it OFF.
// =================================================
#define LED_RED    14
#define LED_GREEN  25
#define LED_BLUE   4

// =================================================
// KY-006 PASSIVE PIEZO, TWO-PIN DRIVE
// S   -> GPIO12
// -   -> GPIO13
// +   -> not connected
//
// The KY-006 module carries a passive piezoelectric element. Unlike an active
// buzzer it produces no sound without an alternating drive signal.
// This project uses a differential two-GPIO arrangement: BUZZER_A and BUZZER_B
// are always driven to opposite logic levels so that the full 3.3V supply
// swing appears across the piezo membrane, improving loudness at 3.3V.
// Neither buzzer terminal in this configuration goes to GND directly —
// the GPIO pins themselves form the differential drive.
// The middle (+) pin on the KY-006 header is left unconnected.
// =================================================
#define BUZZER_A   12
#define BUZZER_B   13

// =================================================
// MOTION DETECTION SETTINGS
// =================================================

// Acceleration magnitude threshold (in g) above which a motion alert fires.
// At complete rest the BMI160 reads approximately 1 g because gravity is
// always present in the measurement.  Only magnitudes well above 1 g
// indicate significant motion.  1.2 g is a very conservative starting point;
// calibrate upward using real Serial Monitor readings before deployment to
// reduce false positives from ordinary movement.
// Start at 2.5 g, then tune using Serial Monitor values.
const float MOTION_THRESHOLD_G = 1.2;

// Duration (milliseconds) the alert LED/display remain red after an event.
// Sender stays red for this long after detection.
const unsigned long ALERT_DISPLAY_MS = 2000;

// Minimum time (milliseconds) between successive alert transmissions.
// Prevents the radio from flooding the channel with repeated packets when
// a sustained motion event keeps the magnitude above the threshold.
// Avoid repeated radio messages from one impact.
const unsigned long ALERT_COOLDOWN_MS = 5000;

// How often (milliseconds) the sensor is read and the OLED is refreshed.
// Serial/OLED sensor update rate.
const unsigned long SENSOR_UPDATE_MS = 500;

// Tracks the last time the sensor was polled.
unsigned long lastSensorUpdate = 0;
// Tracks the last time an alert was triggered, used for cooldown enforcement.
unsigned long lastAlertTime = 0;
// Sequential counter incremented on every alert; included in LoRa packets.
uint32_t alertNumber = 0;

// ------------------------------------------------------------------
// LED helper functions
// Normal (green) state: monitoring is active, no alert in progress.
// ------------------------------------------------------------------

void ledGreen() {
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, HIGH);
}

// Alert (red) state: a motion threshold event has been detected.
void ledRed() {
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
}

// ------------------------------------------------------------------
// OLED display functions
// ------------------------------------------------------------------

// Normal monitoring screen: shows live acceleration magnitude and threshold.
void showMonitoring(float accelerationMagnitude) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("SENDER");

  display.setTextSize(1);
  display.setCursor(0, 23);
  display.println("MONITORING");
  display.print("Acceleration: ");
  display.print(accelerationMagnitude, 2);
  display.println(" g");
  display.print("Trigger: ");
  display.print(MOTION_THRESHOLD_G, 1);
  display.println(" g");
  display.display();
}

// Alert screen: shown during the ALERT_DISPLAY_MS window after an event.
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

// Error screen: displayed when setup() fails to initialize a peripheral.
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

// ------------------------------------------------------------------
// Buzzer functions
// ------------------------------------------------------------------

// Differential buzzer drive: the two GPIO signals remain opposite.
// Produces a square wave of the requested frequency for durationMs by
// toggling both pins together with opposite polarity, maximising the
// voltage swing across the passive piezo element at 3.3V supply.
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

// Alert tone: sweeps between two frequencies near the passive piezo's
// resonant region to produce a noticeable alarm sound.
void buzzerAlarm() {
  // Sweep/alternate close to a passive piezo's usual resonant region.
  for (int i = 0; i < 3; i++) {
    differentialTone(1900, 160);
    differentialTone(2300, 160);
  }

  // Same state on both ends: no voltage across piezo.
  // Setting both pins LOW removes the differential drive, silencing the piezo.
  digitalWrite(BUZZER_A, LOW);
  digitalWrite(BUZZER_B, LOW);
}

// ------------------------------------------------------------------
// LoRa transmission
// ------------------------------------------------------------------

// Builds and transmits a compact LoRa packet containing the alert number
// and the acceleration magnitude that triggered the alert.
// Packet format: "FALL_ALERT,<alertNumber>,MAG=<magnitude>"
void sendFallAlert(float accelerationMagnitude) {
  String message = "FALL_ALERT," + String(alertNumber) +
                   ",MAG=" + String(accelerationMagnitude, 2);

  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();

  Serial.print(">>> SENT: ");
  Serial.println(message);
}

// ------------------------------------------------------------------
// Alert sequence
// ------------------------------------------------------------------

// Called when the motion threshold is exceeded and the cooldown has elapsed.
// Sequence: increment counter → turn LED red → update OLED →
//           transmit LoRa packet → sound buzzer → wait ALERT_DISPLAY_MS →
//           return to green/monitoring state.
// The LoRa packet is sent before the buzzer so radio transmission is not
// delayed by the blocking buzzer routine.
void triggerAlert(float accelerationMagnitude) {
  alertNumber++;
  lastAlertTime = millis();

  // Immediately show physical and display alert.
  ledRed();
  showAlert(accelerationMagnitude);

  // Transmit before the buzzer routine blocks the CPU.
  sendFallAlert(accelerationMagnitude);

  // Sound while LED remains red.
  buzzerAlarm();

  // Keep red for total of about 2 seconds.
  delay(ALERT_DISPLAY_MS);

  // Return to normal green/monitoring state after the alert window expires.
  ledGreen();
  showMonitoring(accelerationMagnitude);
}

// ------------------------------------------------------------------
// setup() — runs once at power-on or reset
//
// Initializes all hardware in dependency order:
//   1. GPIO pins (LED, buzzer) — no bus required.
//   2. I2C bus — needed by both OLED and BMI160.
//   3. OLED — provides status feedback for the remaining setup steps.
//   4. BMI160 — motion sensor needed in loop().
//   5. SPI bus and LoRa radio — radio must be ready before loop() can transmit.
// If any step fails the firmware halts and shows an error on the OLED so
// the problem can be diagnosed before deployment.
// ------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(500);

  // Configure LED pins and start in green (monitoring) state.
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  ledGreen();

  // Configure buzzer pins; start with both LOW (piezo silent).
  pinMode(BUZZER_A, OUTPUT);
  pinMode(BUZZER_B, OUTPUT);
  digitalWrite(BUZZER_A, LOW);
  digitalWrite(BUZZER_B, LOW);

  // OLED and BMI160 share this I2C bus.
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize the built-in SSD1306 OLED. Halts if not found.
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED failed.");
    while (true) {
      delay(1000);
    }
  }

  // Show a startup message while the BMI160 initializes.
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Starting BMI160...");
  display.display();

  // Initialize BMI160 at its I2C address. Halts if not found.
  // Verify VIN, GND, SDA (GPIO21), and SCL (GPIO22) connections if this fails.
  if (bmi160.I2cInit(BMI160_ADDR) != BMI160_OK) {
    Serial.println("BMI160 failed at address 0x69.");
    showError("BMI160 not found", "Check VIN/GND/SDA/SCL.");

    while (true) {
      delay(1000);
    }
  }

  Serial.println("BMI160 initialized at I2C address 0x69.");

  // Configure the hardware SPI bus with the internal LoRa pin mapping,
  // then initialize the SX1276 at the configured band frequency.
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  // Halts if LoRa radio not found. Ensure the antenna is attached and the
  // correct band module is installed before uploading.
  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa failed.");
    showError("LoRa failed", "Check antenna/band.");

    while (true) {
      delay(1000);
    }
  }

  // Radio parameters — must match the receiver exactly.
  LoRa.setTxPower(17);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  Serial.println("Sender is ready.");
  Serial.println("Columns: GX GY GZ (raw) | AX AY AZ (g) | MAG (g)");

  // Show the initial monitoring screen with an approximate resting magnitude.
  showMonitoring(1.0);
}

// ------------------------------------------------------------------
// loop() — runs continuously after setup()
// ------------------------------------------------------------------

void loop() {
  // Rate-limit sensor reads to SENSOR_UPDATE_MS (500 ms by default).
  // Returns early if the interval has not elapsed yet.
  if (millis() - lastSensorUpdate < SENSOR_UPDATE_MS) {
    return;
  }

  lastSensorUpdate = millis();

  // Six-element array to receive gyro and accelerometer raw data.
  int16_t sensorData[6] = {0};

  // Read raw data from BMI160. Skip this cycle on I2C error.
  if (bmi160.getAccelGyroData(sensorData) != BMI160_OK) {
    Serial.println("BMI160 read error.");
    return;
  }

  /*
    DFRobot library data order:
    sensorData[0..2] = Gyroscope X, Y, Z
    sensorData[3..5] = Accelerometer X, Y, Z

    The example library converts acceleration as raw / 16384.0.
  */
  // Gyroscope raw counts (not converted to physical units here).
  float gx = sensorData[0];
  float gy = sensorData[1];
  float gz = sensorData[2];

  // Accelerometer values converted to g using the BMI160 ±2 g sensitivity
  // (16384 LSB/g for the ±2 g range as documented in the DFRobot example).
  float ax = sensorData[3] / 16384.0;
  float ay = sensorData[4] / 16384.0;
  float az = sensorData[5] / 16384.0;

  // Total acceleration magnitude: sqrt(ax² + ay² + az²).
  // At rest this value is approximately 1 g because gravitational acceleration
  // is included in all three axes of a MEMS accelerometer. Only values
  // significantly above 1 g represent dynamic motion. A single threshold on
  // magnitude may cause false positives and is not a validated fall detector.
  float magnitudeG = sqrt(ax * ax + ay * ay + az * az);

  // Print all six raw/converted channels plus the magnitude to Serial Monitor.
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

  // Update the OLED with the current magnitude reading.
  showMonitoring(magnitudeG);

  // Cooldown guard: suppress repeated alerts from a single sustained event.
  // lastAlertTime is 0 at startup, so the first alert fires without waiting.
  bool cooldownFinished =
      (millis() - lastAlertTime >= ALERT_COOLDOWN_MS);

  // Trigger alert only when both conditions are met:
  //   1. Magnitude has exceeded the configured threshold.
  //   2. The alert cooldown period since the last alert has elapsed.
  if (magnitudeG >= MOTION_THRESHOLD_G && cooldownFinished) {
    triggerAlert(magnitudeG);
  }
}
