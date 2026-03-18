/*
 * LoRa Pilot - Transmitter (TX)
 * Platform: ESP32-C3
 * Library: RadioLib (by Jan Gromes)
 *
 * Target: Low Power (Deep Sleep)
 * Wakeup sources: Timer (Keep-alive) or GPIO (Sensor Dry Contact)
 */

#include <RadioLib.h>

// LoRa Pinout for ESP32-C3 (Standard SPI)
#define NSS_PIN 7   // CS
#define DIO1_PIN 1  // G0 / Interrupt
#define NRST_PIN 0  // Reset
#define BUSY_PIN 10 // Optional for SX126x/SX127x (RadioLib Module init)

// Sensor & Battery
#define SENSOR_PIN GPIO_NUM_2 // Terminal block (GPIO 2 + GND)
#define BATTERY_PIN 3         // ADC pin for battery monitoring

// Keep-alive interval: 1 hour (in microseconds)
#define TIME_TO_SLEEP 3600000000ULL

SX1278 radio = new Module(NSS_PIN, DIO1_PIN, NRST_PIN, BUSY_PIN);

// Global state
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool lastSensorState = false;

float readBatteryLevel() {
  // Assuming a voltage divider (e.g., 1M/300k) to bring 9V down to < 3.3V
  // Adjust the multiplier based on your resistor values
  int raw = analogRead(BATTERY_PIN);
  float voltage = (raw / 4095.0) * 3.3 * (1300.0 / 300.0);
  return voltage;
}

void setup() {
  Serial.begin(115200);

  // Configure Sensor Pin with Pull-up
  pinMode(SENSOR_PIN, INPUT_PULLUP);

  // Check wakeup reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  bool currentSensorState = digitalRead(SENSOR_PIN);
  bool isSensorWakeup = (wakeup_reason == ESP_SLEEP_WAKEUP_GPIO);

  // Detect state change: Either physical wakeup or logic mismatch
  bool stateChanged = (currentSensorState != lastSensorState);

  // Initialize LoRa
  Serial.print(F("[LoRa] Initializing ... "));
  int state = radio.begin(433.0);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // Packet construction: [Type, SensorState, BatteryVoltage]
  // Type: 0 = Keep-alive, 1 = Alarm (State Change), 2 = Low Battery
  byte packet[4];
  float batt = readBatteryLevel();

  if (stateChanged) {
    packet[0] = 1; // Alarm (State Change)
    Serial.print(F("Alarm! Sensor is now: "));
    Serial.println(currentSensorState ? "OPEN"
                                      : "CLOSED"); // Logic depends on pull-up
  } else if (batt < 3.2) { // 3.2V is a safe 'low' for Li-SOCl2 3.6V
    packet[0] = 2;         // Low Battery
  } else {
    packet[0] = 0; // Keep-alive
  }

  packet[1] = currentSensorState ? 1 : 0;
  packet[2] = (byte)(batt * 10);
  packet[3] = bootCount % 256;

  // Send packet
  radio.transmit(packet, 4);

  // Update state for next wakeup
  lastSensorState = currentSensorState;
  bootCount++;

  // Prepare for Deep Sleep
  Serial.println(F("Going to sleep now"));

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP);

  // Configure wakeup for ANY change on GPIO
  // Since ESP32-C3 sleep_enable_gpio_wakeup supports levels, we choose the
  // opposite of current
  if (currentSensorState == LOW) {
    esp_deep_sleep_enable_gpio_wakeup(1 << SENSOR_PIN,
                                      ESP_GPIO_WAKEUP_GPIO_HIGH);
  } else {
    esp_deep_sleep_enable_gpio_wakeup(1 << SENSOR_PIN,
                                      ESP_GPIO_WAKEUP_GPIO_LOW);
  }

  radio.sleep();
  esp_deep_sleep_start();
}

void loop() {
  // Execution never reaches here
}
