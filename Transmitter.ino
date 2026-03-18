/*
 * LoRa Pilot - Transmitter (TX)
 * Platform: ESP32-C3
 * Library: RadioLib (by Jan Gromes)
 *
 * Target: Low Power (Deep Sleep)
 * Wakeup sources: Timer (Wire Supervision)
 */

#include <RadioLib.h>

// LoRa Pinout for ESP32-C3 (Standard SPI)
#define NSS_PIN 7   // CS
#define DIO1_PIN 1  // G0 / Interrupt
#define NRST_PIN 0  // Reset
#define BUSY_PIN 10 // Optional for SX126x/SX127x

// Sensor & Battery
#define WIRE_PULSE_PIN GPIO_NUM_4 // Sends the pulse to the wire
#define WIRE_READ_PIN GPIO_NUM_5  // Reads the wire state (pull-down enabled)
#define BATTERY_PIN 3             // ADC pin for battery monitoring

// Intervals
#define TIME_TO_SLEEP 30000000ULL // 30 seconds (in microseconds)
#define KEEP_ALIVE_CYCLES 120     // 120 * 30s = 1 hour

SX1278 radio = new Module(NSS_PIN, DIO1_PIN, NRST_PIN, BUSY_PIN);

// Global state using RTC memory (preserved during Deep Sleep)
RTC_DATA_ATTR int wakeCycleCount = 0;
RTC_DATA_ATTR bool lastWireState = true; // true = OK (Closed)
RTC_DATA_ATTR int bootCount = 0;

float readBatteryLevel() {
  // Assuming a voltage divider (e.g., 1M/300k) but adjusting for 3V AA Lithium
  // Lithium batteries (2xAA) are max 3.0-3.6V. Adjust the multiplier according to your real resistor divider.
  int raw = analogRead(BATTERY_PIN);
  float voltage = (raw / 4095.0) * 3.3 * (1300.0 / 300.0);
  return voltage;
}

void setup() {
  // 1. FAST WIRE CHECK
  // Configured to minimize active time before initializing the LoRa module
  
  pinMode(WIRE_PULSE_PIN, OUTPUT);
  // Se requiere resistencia física pulldown de 1M o usar la interna
  pinMode(WIRE_READ_PIN, INPUT_PULLDOWN);

  // Emit pulse
  digitalWrite(WIRE_PULSE_PIN, HIGH);
  delayMicroseconds(500); // Wait for signal to propagate through 10m-20m wire
  bool currentWireState = digitalRead(WIRE_READ_PIN); // HIGH = wire intact (Closed), LOW = Cut
  digitalWrite(WIRE_PULSE_PIN, LOW);

  // 2. LOGIC EVALUATION
  bool stateChanged = (currentWireState != lastWireState);
  bool isKeepAlive = (wakeCycleCount >= KEEP_ALIVE_CYCLES);

  if (!stateChanged && !isKeepAlive && wakeCycleCount != 0) {
    // Routine check, no changes, go immediately back to sleep to save power!
    lastWireState = currentWireState;
    wakeCycleCount++;
    
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP);
    esp_deep_sleep_start();
  }

  // 3. IF ALARM OR KEEP-ALIVE, INITIALIZE PERIPHERALS
  Serial.begin(115200);
  Serial.println(F("\n[LoRa] Waking up for transmission..."));

  Serial.print(F("[LoRa] Initializing ... "));
  int state = radio.begin(433.0);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // Packet construction: [Type, WireState, BatteryVoltage]
  // Type: 0 = Keep-alive, 1 = Alarm (State Change), 2 = Low Battery
  byte packet[4];
  float batt = readBatteryLevel();

  if (stateChanged) {
    packet[0] = 1; // Alarm (State Change)
    Serial.print(F("Alarm! Wire is now: "));
    Serial.println(currentWireState ? "OK (CLOSED)" : "CUT (OPEN)");
  } else if (batt < 2.5) { // 2.5V is low for 2xAA Lithium in series
    packet[0] = 2;         // Low Battery
  } else {
    packet[0] = 0; // Keep-alive
  }

  packet[1] = currentWireState ? 1 : 0; // 1 = Closed (OK), 0 = Open (CUT)
  packet[2] = (byte)(batt * 10);
  packet[3] = bootCount % 256;

  // Send packet
  radio.transmit(packet, 4);

  // Update state for next wakeup
  lastWireState = currentWireState;
  wakeCycleCount = 1; // Reset keep-alive counter
  bootCount++;

  // Prepare for Deep Sleep
  Serial.println(F("Going to sleep now"));
  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP);
  radio.sleep();
  esp_deep_sleep_start();
}

void loop() {
  // Execution never reaches here
}
