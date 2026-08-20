/*
 * ============================================================
 * ARMOR — Edge-AI Wearable Fall-Detection & LoRa Emergency Alert
 * Firmware role: RECEIVER (base-station / alert unit)
 * Board: LILYGO T3 V1.6.1 (ESP32 + SX1276 LoRa + SSD1306 OLED)
 *
 * Behavior:
 *   - Continuously listens for FALL_ALERT LoRa packets.
 *   - When any valid FALL_ALERT packet arrives:
 *       * LED turns red.
 *       * OLED displays alert number, RSSI, and SNR.
 *       * Buzzer repeats until GPIO 2 button is pressed.
 *   - While the alarm is active, any additional FALL_ALERT packet:
 *       * Updates the OLED information.
 *       * Keeps the LED red and buzzer repeating.
 *   - GPIO 2 button:
 *       * Acknowledges the receiver alarm.
 *       * Stops buzzer.
 *       * Turns LED green.
 *       * Returns OLED to LISTENING.
 *
 * Acknowledge-button wiring:
 *   GPIO 2 ---- normally-open pushbutton ---- GND
 *
 * INPUT_PULLUP behavior:
 *   Not pressed = HIGH
 *   Pressed     = LOW
 *
 * ESP32 Arduino Core 3.x compatible:
 *   Uses ledcAttach(), ledcWriteTone(), and ledcWrite() with the
 *   GPIO pin directly rather than ledcSetup()/ledcAttachPin().
 *
 * This is a prototype, not a medical/safety-certified device.
 * ============================================================
 */

// SPI driver — required for the SX1276 LoRa module via hardware SPI.
#include <SPI.h>
// LoRa packet radio driver by Sandeep Mistry.
#include <LoRa.h>
// I2C driver — used by the built-in OLED.
#include <Wire.h>
// Adafruit GFX graphics primitives — required by the SSD1306 driver.
#include <Adafruit_GFX.h>
// Adafruit SSD1306 OLED driver for the built-in 128×64 display.
#include <Adafruit_SSD1306.h>

// =================================================
// LILYGO T3 V1.6.1 INTERNAL SX1276 LORA PINS
//
// These are hardwired on the T3 V1.6.1 PCB. Do not connect any external
// peripheral to these GPIOs.
// =================================================
#define LORA_SCK   5
#define LORA_MISO  19
#define LORA_MOSI  27
#define LORA_SS    18
#define LORA_RST   23
#define LORA_DIO0  26

// Operating frequency — must match the sender exactly.
// 915E6 = 915 MHz (Australia, North America). Change to 868E6 for EU modules.
#define LORA_BAND  915E6

// =================================================
// BUILT-IN OLED
//
// SSD1306 128×64 panel connected internally on the T3 V1.6.1.
// GPIO21 = SDA, GPIO22 = SCL, I2C address = 0x3C.
// =================================================
#define OLED_SDA   21
#define OLED_SCL   22
#define OLED_ADDR  0x3C
#define SCREEN_W   128
#define SCREEN_H   64

// -1 reset argument: no dedicated hardware reset pin; uses software reset.
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// =================================================
// EXTERNAL COMMON-ANODE RGB LED
//
// Common anode (long leg) -> 3.3V.
// Each cathode connects through a 220–330 Ω resistor to its GPIO pin.
// LOW = color ON (anode higher than cathode); HIGH = color OFF.
// =================================================
#define LED_RED    14
#define LED_GREEN  25
#define LED_BLUE   4

// =================================================
// KY-006 PASSIVE BUZZER — SINGLE-PIN PWM DRIVE
//
// Previous version used a differential two-GPIO drive (both BUZZER_A and
// BUZZER_B toggled opposite). This version uses ESP32 hardware PWM (LEDC)
// on BUZZER_A (GPIO 12) for a clean square wave. BUZZER_B (GPIO 13) is
// held permanently LOW as the piezo return path.
//
// Wiring:
//   KY-006 S / piezo terminal -> GPIO 12  (PWM output)
//   KY-006 - / piezo terminal -> GPIO 13  (held LOW)
//   KY-006 + pin -> unconnected
// =================================================
#define BUZZER_A   12
#define BUZZER_B   13

// =================================================
// ACKNOWLEDGE BUTTON
//
// Wiring: GPIO 2 ---- normally-open pushbutton ---- GND
// Configured INPUT_PULLUP: unpressed = HIGH, pressed = LOW.
// Pressing the button during an active alarm silences it and
// returns the receiver to the LISTENING state.
// =================================================
#define ACK_BUTTON_PIN  2

// Minimum time (ms) a button state must be stable before it is accepted.
// Prevents false triggers from contact bounce.
const unsigned long BUTTON_DEBOUNCE_MS = 50;

// =================================================
// REPEATING ALARM PATTERN
//
// The alarm cycles: tone 1 → tone 2 → silence → repeat.
// Using non-blocking millis() timing so LoRa packets can still be
// received and processed while the alarm is sounding.
// =================================================
const uint16_t ALARM_TONE_1_HZ = 1900;
const uint16_t ALARM_TONE_2_HZ = 2300;

const unsigned long ALARM_TONE_1_MS = 180;
const unsigned long ALARM_TONE_2_MS = 180;
const unsigned long ALARM_SILENCE_MS = 400;

// 8-bit PWM resolution: duty cycle 128 / 255 ≈ 50% square wave.
const int BUZZER_PWM_RESOLUTION = 8;

// =================================================
// Runtime state
// =================================================

// True when a FALL_ALERT has been received and not yet acknowledged.
bool alarmActive = false;

// Counts all received packets (any type) and validated FALL_ALERT packets.
uint32_t receivedPacketCount = 0;
uint32_t receivedAlertCount = 0;

// Stores the most recently received FALL_ALERT data for OLED display.
String latestMessage = "";
int latestAlertNumber = -1;
float latestMagnitude = 0.0;
int latestRssi = 0;
float latestSnr = 0.0;

// Button debounce state variables.
bool lastRawButtonReading = HIGH;
bool stableButtonState = HIGH;
unsigned long lastDebounceTime = 0;

// =================================================
// Buzzer state — non-blocking alarm sequencer
//
// The alarm cycles through three phases without using delay().
// This keeps loop() responsive to incoming LoRa packets and button presses
// while the alarm is running.
// =================================================
enum AlarmTonePhase {
  ALARM_TONE_1,
  ALARM_TONE_2,
  ALARM_SILENCE
};

AlarmTonePhase alarmTonePhase = ALARM_TONE_1;
unsigned long alarmPhaseStartTime = 0;

// =================================================
// LED functions
// =================================================

// Normal state: receiver is listening, no active alarm.
void ledGreen() {
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, HIGH);
}

// Alert state: a FALL_ALERT packet has been received and is unacknowledged.
void ledRed() {
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
}

// =================================================
// Buzzer functions
//
// Uses the ESP32 Arduino Core 3.x LEDC API: ledcAttach(), ledcWriteTone(),
// and ledcWrite() take the GPIO pin number directly (no separate channel
// setup required). The channel is allocated internally by the driver.
// =================================================

// Start a continuous tone at the given frequency using hardware PWM.
void buzzerStart(uint16_t frequencyHz) {
  ledcWriteTone(BUZZER_A, frequencyHz);
  // 128 / 255 ≈ 50% duty cycle for maximum piezo excitation.
  ledcWrite(BUZZER_A, 128);
}

// Stop the buzzer by zeroing the duty cycle, then the frequency.
void buzzerStop() {
  ledcWrite(BUZZER_A, 0);
  ledcWriteTone(BUZZER_A, 0);
}

// =================================================
// OLED functions
// =================================================

// Idle screen: shown at startup and after each alarm is acknowledged.
void showListening() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("RECEIVER");

  display.setTextSize(1);
  display.setCursor(0, 23);
  display.println("Status: LISTENING");
  display.println("LED: GREEN");
  display.print("Alerts RX: ");
  display.println(receivedAlertCount);
  display.print("Packets RX: ");
  display.println(receivedPacketCount);

  display.display();
}

// Active alarm screen: shows alert number, RSSI, SNR, and ACK prompt.
// RSSI (dBm): less negative = stronger signal.
// SNR (dB): positive = signal above noise floor.
void showActiveAlert() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("FALL ALERT");

  display.setTextSize(1);
  display.setCursor(0, 18);
  display.println("ALARM ACTIVE");

  display.print("Alert #: ");
  display.println(latestAlertNumber);

  display.print("RSSI: ");
  display.print(latestRssi);
  display.print(" SNR:");
  display.println(latestSnr, 1);

  display.print("Alerts RX: ");
  display.println(receivedAlertCount);

  // Bottom-row prompt reminds the operator which button to press.
  display.setCursor(0, 56);
  display.println("Press GPIO2 ACK");

  display.display();
}

// Brief confirmation screen shown for ~800 ms after the button is pressed.
void showAcknowledged() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("ACKNOWLEDGED");

  display.setTextSize(1);
  display.setCursor(0, 28);
  display.println("Alarm silenced");
  display.println("Returning to listen...");
  display.display();
}

// Error screen shown when LoRa initialization fails in setup().
void showLoRaError() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("ERROR");

  display.setTextSize(1);
  display.setCursor(0, 27);
  display.println("LoRa failed.");
  display.println("Check antenna");
  display.println("and LoRa band.");

  display.display();
}

// =================================================
// Packet parsing
//
// Sender packet format:
//   FALL_ALERT,<alertNumber>,MAG=<magnitude>
//
// Example:
//   FALL_ALERT,7,MAG=2.31
// =================================================

// Extracts the sequential alert number from the packet string.
// Returns -1 if the format is unrecognised.
int parseAlertNumber(const String &message) {
  const String prefix = "FALL_ALERT,";

  if (!message.startsWith(prefix)) {
    return -1;
  }

  int secondCommaIndex = message.indexOf(',', prefix.length());

  if (secondCommaIndex == -1) {
    return -1;
  }

  String alertNumberText =
      message.substring(prefix.length(), secondCommaIndex);

  return alertNumberText.toInt();
}

// Extracts the acceleration magnitude (in g) from the "MAG=" field.
// Returns 0.0 if the field is not found.
float parseMagnitude(const String &message) {
  int magnitudeIndex = message.indexOf("MAG=");

  if (magnitudeIndex == -1) {
    return 0.0;
  }

  String magnitudeText = message.substring(magnitudeIndex + 4);
  return magnitudeText.toFloat();
}

// =================================================
// Alarm control
// =================================================

// Activates the repeating alarm: sets the alarm flag, starts the first
// tone phase, turns LED red, and begins the buzzer.
void startAlarm() {
  alarmActive = true;

  alarmTonePhase = ALARM_TONE_1;
  alarmPhaseStartTime = millis();

  ledRed();
  buzzerStart(ALARM_TONE_1_HZ);
}

// Deactivates the alarm: clears the flag, stops the buzzer, returns LED green.
void stopAlarm() {
  alarmActive = false;

  buzzerStop();
  ledGreen();

  Serial.println(">>> Alarm acknowledged locally.");
}

// Called every loop() iteration. Advances the buzzer through its
// tone1 → tone2 → silence → repeat cycle using non-blocking timing.
// Does nothing if no alarm is currently active.
void updateAlarmBuzzer() {
  if (!alarmActive) {
    return;
  }

  unsigned long now = millis();
  unsigned long phaseElapsed = now - alarmPhaseStartTime;

  switch (alarmTonePhase) {
    case ALARM_TONE_1:
      if (phaseElapsed >= ALARM_TONE_1_MS) {
        alarmTonePhase = ALARM_TONE_2;
        alarmPhaseStartTime = now;
        buzzerStart(ALARM_TONE_2_HZ);
      }
      break;

    case ALARM_TONE_2:
      if (phaseElapsed >= ALARM_TONE_2_MS) {
        alarmTonePhase = ALARM_SILENCE;
        alarmPhaseStartTime = now;
        buzzerStop();
      }
      break;

    case ALARM_SILENCE:
      if (phaseElapsed >= ALARM_SILENCE_MS) {
        alarmTonePhase = ALARM_TONE_1;
        alarmPhaseStartTime = now;
        buzzerStart(ALARM_TONE_1_HZ);
      }
      break;
  }
}

// Returns true exactly once for each stable HIGH-to-LOW transition on
// ACK_BUTTON_PIN (i.e. each physical button press after debounce).
// Must be called every loop() iteration to track edge transitions correctly.
bool acknowledgeButtonPressed() {
  bool rawButtonReading = digitalRead(ACK_BUTTON_PIN);

  // Reset the debounce timer whenever the raw reading changes.
  if (rawButtonReading != lastRawButtonReading) {
    lastDebounceTime = millis();
  }

  bool buttonPressed = false;

  if (millis() - lastDebounceTime >= BUTTON_DEBOUNCE_MS) {
    if (rawButtonReading != stableButtonState) {
      stableButtonState = rawButtonReading;

      // INPUT_PULLUP: LOW = pressed.
      if (stableButtonState == LOW) {
        buttonPressed = true;
      }
    }
  }

  lastRawButtonReading = rawButtonReading;
  return buttonPressed;
}

// =================================================
// Valid fall-alert handling
// =================================================

// Called when a packet starting with "FALL_ALERT," is received.
// Parses the alert number and magnitude, increments the alert counter,
// and either starts a new alarm or refreshes the OLED if already active.
// A single acceleration threshold may cause false positives;
// this system is not a validated fall detector.
void handleFallAlert(const String &message, int rssi, float snr) {
  int receivedAlertNumber = parseAlertNumber(message);

  if (receivedAlertNumber < 0) {
    Serial.println("Ignored: malformed FALL_ALERT packet.");
    return;
  }

  latestMessage = message;
  latestAlertNumber = receivedAlertNumber;
  latestMagnitude = parseMagnitude(message);
  latestRssi = rssi;
  latestSnr = snr;

  // Every valid FALL_ALERT increases the received alert count.
  receivedAlertCount++;

  Serial.print(">>> FALL_ALERT #");
  Serial.print(latestAlertNumber);
  Serial.println(" received.");

  // Only initialize the repeating buzzer sequence on the first packet
  // that activates an alarm. Additional alerts just refresh the OLED.
  if (!alarmActive) {
    startAlarm();
  } else {
    // Alarm is already running; keep the LED red and refresh the display
    // with the updated alert number, RSSI, and SNR.
    ledRed();
  }

  showActiveAlert();
}

// =================================================
// LoRa receive processing
// =================================================

// Non-blocking LoRa poll. Reads one complete packet per call if available.
// Increments the packet counter for all received packets regardless of type.
// Only FALL_ALERT packets trigger the alarm; all others are logged and ignored.
void checkLoRaPackets() {
  int packetSize = LoRa.parsePacket();

  if (packetSize == 0) {
    return;
  }

  String message = "";

  while (LoRa.available()) {
    message += (char)LoRa.read();
  }

  int rssi = LoRa.packetRssi();
  float snr = LoRa.packetSnr();

  receivedPacketCount++;

  Serial.print("Received: ");
  Serial.print(message);
  Serial.print(" | RSSI: ");
  Serial.print(rssi);
  Serial.print(" dBm | SNR: ");
  Serial.print(snr, 1);
  Serial.println(" dB");

  if (message.startsWith("FALL_ALERT,")) {
    handleFallAlert(message, rssi, snr);
  } else {
    Serial.println("Ignored: packet is not a FALL_ALERT.");
  }
}

// =================================================
// setup() — runs once at power-on or reset
//
// Initialization order:
//   1. GPIO (LED, button, buzzer) — no bus required.
//   2. LEDC PWM channel attached to BUZZER_A.
//   3. I2C bus, then OLED.
//   4. SPI bus, then LoRa radio.
//
// Halts with an error screen if OLED or LoRa init fails so the fault
// can be identified before deployment.
// =================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  // RGB LED — start green (listening / ready).
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  ledGreen();

  // Acknowledge button — pulled HIGH internally; button pulls to GND.
  pinMode(ACK_BUTTON_PIN, INPUT_PULLUP);

  // Buzzer — GPIO13 stays LOW as the piezo return path.
  // GPIO12 carries the LEDC PWM waveform.
  pinMode(BUZZER_B, OUTPUT);
  digitalWrite(BUZZER_B, LOW);

  // Attach LEDC to BUZZER_A using the ESP32 Arduino Core 3.x API.
  // No explicit channel number needed — the driver allocates it internally.
  ledcAttach(BUZZER_A, ALARM_TONE_1_HZ, BUZZER_PWM_RESOLUTION);
  buzzerStop();

  // OLED — initialize I2C then start the display.
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED initialization failed.");

    while (true) {
      delay(1000);
    }
  }

  // Show startup message while LoRa initializes.
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Starting receiver...");
  display.display();

  // LoRa — configure SPI pins then initialize the radio.
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa initialization failed.");
    showLoRaError();

    while (true) {
      delay(1000);
    }
  }

  // Radio parameters — must match sender exactly for packets to be decoded.
  // Must match sender settings exactly.
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  Serial.println("Receiver ready.");
  Serial.println("GPIO2 acknowledges an active FALL_ALERT alarm.");

  // Radio is now in continuous receive mode. showListening() reflects this.
  showListening();
}

// =================================================
// loop() — runs continuously after setup()
//
// Three tasks run every iteration:
//   1. checkLoRaPackets() — non-blocking poll for new LoRa packets.
//   2. updateAlarmBuzzer() — advances the buzzer phase state machine.
//   3. Button check — silences the alarm when GPIO 2 is pressed.
//
// None of these use delay(), so the loop remains responsive to all three
// inputs simultaneously during an active alarm.
// =================================================
void loop() {
  // This runs during both normal listening and an active alarm.
  checkLoRaPackets();

  // Drives the repeated buzzer sequence without delay().
  updateAlarmBuzzer();

  // Acknowledge only works during an active alarm.
  if (alarmActive && acknowledgeButtonPressed()) {
    stopAlarm();
    showAcknowledged();

    // Brief confirmation pause before returning to listening screen.
    delay(800);

    showListening();
  }
}
