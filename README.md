# LoRa Transmitter-Receiver Pilot

## Overview
This project is an ultra-low-power ESP32-C3 LoRa-based system designed for wire supervision and state change detection.
The transmitter is a compact, battery-powered device that continuously monitors a 10m to 20m 32 AWG enameled copper wire loop using an ultra-low-power fast-boot strategy. It checks the loop every 30 seconds, returning to deep sleep in milliseconds if intact, thereby extending battery life. 

The receiver node collects the LoRa packets and provides a dry contact output capable of interfacing with standard alarm panels, ensuring seamless integration into existing security infrastructures.

## Key Features
- **Ultra-Low Power Wire Supervision**: Checks a 10-20m wire loop every 30 seconds without initializing the LoRa radio unless a change is detected.
- **Exceptional Battery Life**: Optimized to run for over 1 year (theoretically up to 7+ years) on two standard interchangeable AA Lithium batteries (3.0V, ~3000 mAh).
- **Keep-Alive Mechanism**: The transmitter periodically sends a keep-alive signal (every 1 hour) to ensure system integrity.
- **Immediate Alerting**: Instantly reports critical events including wire cut/tamper, low battery (<2.5V), and loss of keep-alive signals at the receiver.
- **Reliable Range**: Achieves 100m+ transmission range using SX1278 (433 MHz).

## Components
- **Transmitter (ESP32-C3)**: Wakes up every 30s to pulse the supervision wire. Only powers up the LoRa module when a state change occurs (wire cut/closed) or during the 1-hour keep-alive interval.
- **Receiver (ESP32)**: Listens for incoming LoRa packets, verifies wire status to drive a physical relay, and monitors timeouts to trigger comms-loss alarms.
