# STM32F4 CAN Telemetry Engine 🚀

A high-performance, register-level embedded telemetry engine implemented on an ARM Cortex-M4 (STM32F407VG) microcontroller. Built entirely without HAL (Hardware Abstraction Layer) or third-party middleware, this project integrates direct register manipulation for SPI communication, an external MCP2515 CAN controller, multi-channel ADC sensor acquisition, and a closed-loop telemetry pipeline.

---

## 🚀 Key Features

* **Pure Bare-Metal Architecture:** Zero HAL dependencies. Direct memory-mapped register manipulation for RCC, GPIO (MODER, AFRL), SPI1, and ADC1.
* **Optimized MCP2515 CAN Integration:** Hardware-verified SPI communication configuring an external MCP2515 CAN controller for 500 kbps (using an 8 MHz crystal).
* **Multi-Channel ADC Acquisition:** Sequential sampling of analog sensors (LM35 Centigrade Temperature Sensor on PA0, Potentiometer on PA1).
* **Deterministic Packed Telemetry Framing:** 3-byte memory-aligned telemetry frames optimized using `__attribute__((packed))` to prevent padding overhead.
* **Hardware Self-Test Verification:** Integrated internal loopback mode validation (`CANSTAT` = `0x40`) observable via ST-Link Live Expressions, guaranteeing end-to-end packet integrity.

---

## 📌 Hardware Pinout & Connections

| Component | Pin / Channel | Connected To | Description |
| :--- | :--- | :--- | :--- |
| **MCP2515 CS** | Chip Select | STM32 **PA4** (Output) | SPI1 Slave Select line |
| **SPI1 SCK** | Clock | STM32 **PA5** (AF5) | SPI Serial Clock |
| **SPI1 MISO** | Master In / Slave Out | STM32 **PA6** (AF5) | SPI Data Reception |
| **SPI1 MOSI** | Master Out / In | STM32 **PA7** (AF5) | SPI Data Transmission |
| **LM35 Sensor** | Signal (Pin 2) | STM32 **PA0** (ADC1_IN0) | Analog temperature input |
| **Potentiometer** | Wiper (Pin 2) | STM32 **PA1** (ADC1_IN1) | Analog throttle/voltage input |
| **Common Ground** | STM32 **GND** | MCP2515 & Sensors | Mandatory common reference rail |
| **Power Distribution**| STM32 **3V3** | Breadboard (+) Rail | Regulated logic & sensor supply |

---

## 📂 Architectural Overview

Project Directory Structure:
- Core/Src/main.c : Super-loop scheduler, register-level SPI, ADC & CAN loopback logic
- README.md : Project documentation

---

## 📊 Telemetry Frame Structures

### Compact Telemetry Packet (Telemetry_Packet_t)
The system packs real-time sensor data into an efficient 3-byte structure transmitted over the CAN bus:

- Byte 0: `counter` (`uint8_t`) -> Rolling packet counter (0 to 255)
- Byte 1: `temperature` (`uint8_t`) -> LM35 converted temperature (°C)
- Byte 2: `potentiometer` (`uint8_t`) -> Scaled potentiometer / throttle input

---

## 🛠️ Environment & Toolchain Setup

* **MCU:** STM32F407VG (ARM Cortex-M4)
* **CAN Controller:** MCP2515 (8 MHz Crystal)
* **Toolchain:** STM32CubeIDE / GNU ARM Embedded Toolchain (`arm-none-eabi-gcc`)
* **Hardware Debugger:** ST-Link V2 (SWD) with real-time Live Expressions inspection

---

## 🚀 Roadmap / Next Steps
- [ ] Implement interrupt-driven reception using the MCP2515 **INT** pin mapped to STM32 EXTI.
- [ ] Switch from Loopback Mode to Normal Mode for multi-node physical bus communication.
- [ ] Integrate GPS module, HC-SR04 ultrasonic sensor, and warning buzzer.
- [ ] Add UART telemetry stream for Ground Station / UI visualization.```