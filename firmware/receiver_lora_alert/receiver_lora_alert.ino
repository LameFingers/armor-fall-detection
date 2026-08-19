/*
 * ============================================================
 * ARMOR — Edge-AI Wearable Fall-Detection & LoRa Emergency Alert
 * Firmware role: RECEIVER (base-station / alert unit)
 * Board: LILYGO T3 V1.6.1 (ESP32 + built-in SX1276 LoRa + built-in SSD1306 OLED)
 *
 * What this firmware does:
 *   - Keeps the SX1276 LoRa radio in continuous receive mode.
 *   - When a "FALL_ALERT,..." packet arrives from the sender, it:
 *       - Turns the LED red.
 *       - Sounds the KY-006 passive buzzer alarm.
 *       - Displays the alert, packet RSSI, and SNR on the OLED.
 *       - Waits two seconds, then returns LED to green and OLED to
 *         "LISTENING" state, ready for the next packet.
 *   - Packets that do not start with "FALL_ALERT," are logged to
 *     Serial Monitor and ignored.
 *   - RSSI (Received Signal Strength Indicator, dBm) and SNR
 *     (Signal-to-Noise Ratio, dB) are read from the SX1276 after each
 *     packet and displayed on screen, providing a useful measure of
 *     link quality during range testing.
 *
 * This firmware has no BMI160 dependency — motion sensing is handled
 * entirely on the sender board.
 *
 * This firmware is a working baseline prototype. It has been physically
 * tested but is NOT medically certified or safety-certified in any way.
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

// MUST match the sender's LoRa band.
// Use 915E6 for 915 MHz modules, 868E6 for 868 MHz modules.
#define LORA_BAND  915E6

// =================================================
// BUILT-IN OLED
// Your board diagram shows SDA = GPIO21 and SCL = GPIO22.
//
// The SSD1306 OLED panel on the T3 V1.6.1 is connected internally to
// the ESP32's default I2C bus.
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
// EXTERNAL COMMON-ANODE RGB LED
//
// Common/long LED leg -> 3.3V
// Red cathode         -> GPIO14 through 220-330 ohm resistor
// Green cathode       -> GPIO25 through 220-330 ohm resistor
// Blue cathode        -> GPIO4 through 220-330 ohm resistor
//
// Common-anode behavior:
// LOW  = color ON
// HIGH = color OFF
//
// A common-anode RGB LED has its shared anode connected to 3.3V.
// Each color cathode connects through a 220–330 ohm current-limiting
// resistor to the GPIO pin below.  Pulling a GPIO LOW forward-biases
// that color's diode (ON); HIGH turns it OFF.
// =================================================
#define LED_RED    14
#define LED_GREEN  25
#define LED_BLUE   4

// =================================================
// KY-006 PASSIVE BUZZER: TWO-GPIO DIFFERENTIAL DRIVE
//
// KY-006 S pin -> GPIO12
// KY-006 - pin -> GPIO13
// KY-006 + pin -> leave unconnected
//
// Do NOT connect either of these two buzzer pins to GND.
//
// This project drives the passive piezo from two GPIO pins with
// opposite output levels (differential drive) to maximize the voltage
// swing across the piezo membrane at 3.3V supply.  Neither of the two
// buzzer terminals in this configuration goes directly to GND.
// =================================================
#define BUZZER_A   12
#define BUZZER_B   13

// Running count of FALL_ALERT packets received in the current session.
uint32_t receivedAlertCount = 0;

// ---------- RGB LED functions ----------

// Normal (green) state: receiver is listening, no alert active.
void ledGreen() {
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, HIGH);
}

// Alert (red) state: a FALL_ALERT packet has been received.
void ledRed() {
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
}

// ---------- OLED functions ----------

// Normal listening screen shown during idle state and after each alert clears.
void showListening() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("RECEIVER");

  display.setTextSize(1);
  display.setCursor(0, 24);
  display.println("Status: LISTENING");
  display.println("LED: GREEN");
  display.print("Alerts RX: ");
  display.println(receivedAlertCount);

  display.display();
}

// Alert screen shown while the receiver is responding to a FALL_ALERT packet.
// Displays the raw packet string, RSSI (dBm), and SNR (dB) for link diagnostics.
// RSSI: higher (less negative) values indicate a stronger signal.
// SNR:  values above 0 dB mean signal is stronger than the noise floor.
void showFallAlert(const String &message, int rssi, float snr) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("FALL ALERT");

  display.setTextSize(1);
  display.setCursor(0, 23);
  display.println("SIGNAL RECEIVED");
  display.print("RSSI: ");
  display.print(rssi);
  display.println(" dBm");

  display.print("SNR: ");
  display.print(snr, 1);
  display.println(" dB");

  display.print("Alert #: ");
  display.println(receivedAlertCount);

  display.display();
}

// Error screen shown if LoRa initialization fails during setup().
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

// ---------- Buzzer functions ----------

// Both buzzer pins are driven oppositely to increase voltage across
// the passive piezo element.
// Produces a square wave of the requested frequency for durationMs by
// toggling both pins together with opposite polarity.
void differentialTone(uint16_t frequencyHz, uint16_t durationMs) {
  unsigned long halfPeriodUs = 500000UL / frequencyHz;
  unsigned long startTime = millis();

  while (millis() - startTime < durationMs) {
    digitalWrite(BUZZER_A, HIGH);
    digitalWrite(BUZZER_B, LOW);
    delayMicroseconds(halfPeriodUs);

    digitalWrite(BUZZER_A, LOW);
    digitalWrite(BUZZER_B, HIGH);
    delayMicroseconds(halfPeriodUs);
  }
}

// Alert tone: six short alternating tones for a noticeable alarm.
void loudReceiverAlarm() {
  // Six short alternating tones for a noticeable alert.
  for (int i = 0; i < 3; i++) {
    differentialTone(1900, 180);
    differentialTone(2300, 180);
  }

  // Equal voltage on both buzzer wires = silent.
  // Setting both pins LOW removes the differential drive, silencing the piezo.
  digitalWrite(BUZZER_A, LOW);
  digitalWrite(BUZZER_B, LOW);
}

// ---------- Fall alert handling ----------

// Full alert response sequence:
//   Increment counter → turn LED red → update OLED with RSSI/SNR →
//   sound buzzer → wait 2 s → return LED to green and OLED to listening.
// After delay(2000) the receiver is silent and ready for the next packet.
void receivedFallAlert(const String &message, int rssi, float snr) {
  receivedAlertCount++;

  // Immediately show visible alarm.
  ledRed();
  showFallAlert(message, rssi, snr);

  // Sound buzzer.
  loudReceiverAlarm();

  // Keep red on for a full visible two seconds.
  delay(2000);

  // Return receiver to its normal ready state.
  // The OLED returns to the LISTENING screen and the LED returns to green,
  // indicating the receiver is back in continuous packet-listening mode.
  ledGreen();
  showListening();
}

// ---------- Setup ----------

/*
 * setup() — runs once at power-on or reset.
 *
 * Initializes all hardware in dependency order:
 *   1. GPIO pins (LED, buzzer) — no bus required.
 *   2. I2C bus — needed by OLED.
 *   3. OLED — provides status feedback for remaining setup steps.
 *   4. SPI bus and LoRa radio — must be ready before loop() can receive.
 *
 * If OLED or LoRa initialization fails the firmware halts and shows an
 * error so the problem can be diagnosed before deployment.
 */

void setup() {
  Serial.begin(115200);
  delay(500);

  // RGB LED setup — start in green (listening) state.
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  ledGreen();

  // Buzzer setup — start with both pins LOW (piezo silent).
  pinMode(BUZZER_A, OUTPUT);
  pinMode(BUZZER_B, OUTPUT);
  digitalWrite(BUZZER_A, LOW);
  digitalWrite(BUZZER_B, LOW);

  // OLED setup — initialize I2C bus then start the display.
  Wire.begin(OLED_SDA, OLED_SCL);

  // Halts if OLED is not found. Verify GPIO21/22 wiring and I2C address 0x3C.
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

  // LoRa setup — configure SPI pins then initialize the radio.
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  // Halts if LoRa radio is not found. Ensure the antenna is attached and
  // LORA_BAND matches the sender and the physical module frequency.
  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa initialization failed.");
    showLoRaError();

    while (true) {
      delay(1000);
    }
  }

  // Radio parameters — must match sender parameters exactly for packets to
  // be received: spreading factor, bandwidth, and coding rate must all agree.
  // Must match sender parameters.
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  Serial.println("Receiver ready.");
  Serial.println("Waiting for FALL_ALERT packets.");

  // Show the initial listening screen; receiver is now in continuous RX mode.
  showListening();
}

// ---------- Main loop ----------

/*
 * loop() — runs continuously after setup().
 *
 * LoRa.parsePacket() is non-blocking: it returns 0 when no complete packet
 * has arrived, allowing the loop to run at full speed without blocking.
 * When a packet arrives, its bytes are read, RSSI and SNR are sampled from
 * the SX1276 registers, the content is checked, and a FALL_ALERT is handled
 * if the prefix matches.  Any other packet is logged and discarded.
 * After handling the alert the receiver re-enters this loop and
 * LoRa.parsePacket() automatically resumes listening — no explicit
 * mode change is required.
 */

void loop() {
  // Non-blocking check for a received LoRa packet.
  // Returns the packet length in bytes, or 0 if nothing has arrived.
  int packetSize = LoRa.parsePacket();

  // No packet available — return immediately to keep polling.
  if (packetSize == 0) {
    return;
  }

  // Read all available bytes into a String.
  String message = "";

  while (LoRa.available()) {
    message += (char)LoRa.read();
  }

  // RSSI: Received Signal Strength Indicator in dBm.
  // More negative = weaker signal; values above about -100 dBm are usable.
  int rssi = LoRa.packetRssi();
  // SNR: Signal-to-Noise Ratio in dB from the SX1276.
  // Positive SNR = signal above noise floor; negative = signal below noise.
  float snr = LoRa.packetSnr();

  Serial.print("Received: ");
  Serial.print(message);

  Serial.print(" | RSSI: ");
  Serial.print(rssi);
  Serial.print(" dBm");

  Serial.print(" | SNR: ");
  Serial.print(snr, 1);
  Serial.println(" dB");

  // Sender sends: FALL_ALERT,<number>,MAG=<acceleration>
  // Filter for valid FALL_ALERT packets; ignore everything else.
  if (message.startsWith("FALL_ALERT,")) {
    receivedFallAlert(message, rssi, snr);
  } else {
    Serial.println("Ignored: packet is not a FALL_ALERT.");
  }
}
