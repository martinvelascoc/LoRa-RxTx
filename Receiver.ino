/*
 * LoRa Pilot - Receiver (RX)
 * Platform: ESP32 (General)
 * Library: RadioLib
 *
 * Target: Mains powered
 * Responsibilities:
 * 1. Maintain sensor state output (Relay/Optocoupler)
 * 2. Monitor Keep-alive timeout
 * 3. Alert on Low Battery or Comms Loss
 */

#include <RadioLib.h>

// LoRa Pinout (Standard ESP32 SPI)
#define NSS_PIN 5
#define SCK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23
#define DIO1_PIN 2  // Interrupt
#define NRST_PIN 14 // Reset

// Output Pins
#define OUTPUT_RELAY_PIN 12 // Salida Bornera (Contacto Seco)
#define COMMS_LOSS_PIN 13   // Led/Alarma Pérdida Keep-alive
#define LOW_BATT_PIN 27     // Led Batería Baja TX

SX1278 radio = new Module(NSS_PIN, DIO1_PIN, NRST_PIN, RADIOLIB_NC);

// Monitoring
unsigned long lastKeepAlive = 0;
const unsigned long TIMEOUT_THRESHOLD =
    4000000; // 4000 seconds (~1.1 hours) to allow jitter

void setup() {
  Serial.begin(115200);

  pinMode(OUTPUT_RELAY_PIN, OUTPUT);
  pinMode(COMMS_LOSS_PIN, OUTPUT);
  pinMode(LOW_BATT_PIN, OUTPUT);

  digitalWrite(OUTPUT_RELAY_PIN, LOW);
  digitalWrite(COMMS_LOSS_PIN, LOW);
  digitalWrite(LOW_BATT_PIN, LOW);

  // Initialize LoRa
  Serial.print(F("[LoRa] Initializing ... "));
  int state = radio.begin(433.0);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
      ;
  }

  lastKeepAlive = millis();
}

void loop() {
  Serial.print(F("[LoRa] Waiting for packet ... "));

  byte packet[4];
  int state = radio.receive(packet, 4);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
    lastKeepAlive = millis();
    digitalWrite(COMMS_LOSS_PIN, LOW); // Reset loss indicator

    byte type = packet[0];
    bool sensorState = (packet[1] == 1);
    float battVoltage = packet[2] / 10.0;

    Serial.print(F("Type: "));
    Serial.println(type);
    Serial.print(F("Sensor: "));
    Serial.println(sensorState ? "CLOSED (OK)" : "OPEN (CUT)");
    Serial.print(F("Battery: "));
    Serial.print(battVoltage);
    Serial.println("V");

    // Reflect sensor state to physical output
    digitalWrite(OUTPUT_RELAY_PIN, sensorState ? HIGH : LOW);

    // Handle Low Battery Alert
    if (type == 2 || battVoltage < 2.5) {
      digitalWrite(LOW_BATT_PIN, HIGH);
    } else {
      digitalWrite(LOW_BATT_PIN, LOW);
    }

  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // Just a timeout on the listening window, no error
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // Check for Comms Loss (Missing Keep-alive)
  if (millis() - lastKeepAlive > TIMEOUT_THRESHOLD) {
    Serial.println(F("CRITICAL: KEEP-ALIVE LOST!"));
    digitalWrite(COMMS_LOSS_PIN, HIGH);
    // Depending on requirements, we might want to force the relay to "open" or
    // "closed" on loss
  }
}
