# LoRa Transmitter-Receiver Pilot

## Overview
This project is an ultra-low-power LoRa-based transmitter and receiver system designed for indoor use. 
The transmitter is a compact, battery-powered device intended to be placed inside a sensor housing (such as a door/window sensor). It detects state changes (open/closed) via a dry contact and transmits this information wirelessly to a central receiver.

The receiver node collects these signals and provides an output capable of interfacing with standard alarm panels, ensuring seamless integration into existing security or monitoring infrastructures.

## Key Features
- **Ultra-Low Power Transmitter**: Designed for a battery life of at least one year with up to 50 activations annually.
- **Reliable Indoor Range**: Achieves 100m+ transmission range, effectively penetrating at least one indoor wall.
- **Keep-Alive Mechanism**: The transmitter periodically sends a keep-alive signal to ensure system integrity.
- **Immediate Alerting**: Instantly reports critical events including sensor state changes, low battery conditions, and loss of keep-alive signals.
- **Flexible Power Options**: The transmitter can potentially be powered directly from an existing sensor's battery (3V or 9V).

## Components
- **Transmitter**: Monitors the dry contact, manages low-power sleep states, and sends LoRa packets upon state change or keep-alive intervals.
- **Receiver**: Listens for incoming LoRa packets, verifies device status, and triggers the physical alarm panel interface.
