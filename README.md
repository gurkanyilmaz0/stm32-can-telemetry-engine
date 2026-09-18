# STM32 Bare-Metal CAN & Wireless Telemetry Engine 🚀

A high-performance, register-level embedded telemetry engine implemented on an ARM Cortex-M4 (STM32F407VG) microcontroller. Built entirely without HAL (Hardware Abstraction Layer) or third-party middleware, this project integrates direct register manipulation for CAN communication, multi-channel ADC sensor acquisition, EXTI interrupts, and a real-time wireless Ground Control Station (GCS) gateway via an ESP8266 (Lolin) node.

---

## 🚀 Key Features

* **Pure Bare-Metal Architecture:** Zero HAL dependencies. Direct memory-mapped register manipulation for RCC, GPIO (MODER, AFRL), SPI1, ADC1, USART2, and EXTI.
* **Optimized MCP2515 CAN Integration:** Hardware-verified SPI communication configuring an external MCP2515 CAN controller for 500 kbps (using an 8 MHz crystal) in loopback mode with custom filter bypass.
* **Multi-Channel ADC Acquisition:** Sequential sampling of analog sensors (LM35 Centigrade Temperature Sensor on PA0, Potentiometer on PA1).
* **Deterministic Packed Telemetry Framing:** 4-byte memory-aligned telemetry frames (`__attribute__((packed))`) containing rolling counter, sensor data, and a custom XOR-based checksum (CRC).
* **Interrupt-Driven Reception:** External interrupt (`EXTI0` on `PB0`) triggered instantly upon packet arrival from the MCP2515 `INT` pin.
* **Wireless Ground Station (GCS) Dashboard:** Real-time data bridging via UART2 to an ESP8266 (Lolin) acting as a Wi-Fi Access Point, rendering a professional dark-themed web dashboard with live telemetry indicators.

---

## 📌 Hardware Pinout & Connections

| Component | Pin / Channel | Connected To | Description |
| :--- | :--- | :--- | :--- |
| **MCP2515 CS** | Chip Select | STM32 **PA4** (Output) | SPI1 Slave Select line |
| **SPI1 SCK** | Clock | STM32 **PA5** (AF5) | SPI Serial Clock |
| **SPI1 MISO** | Master In / Slave Out | STM32 **PA6** (AF5) | SPI Data Reception |
| **SPI1 MOSI** | Master Out / In | STM32 **PA7** (AF5) | SPI Data Transmission |
| **MCP2515 INT** | Interrupt | STM32 **PB0** (EXTI0) | Falling-edge packet arrival interrupt |
| **UART2 TX** | Transmit | ESP8266 (Lolin) **RX** | Telemetry bridge data stream (115200 Baud) |
| **LM35 Sensor** | Signal (Pin 2) | STM32 **PA0** (ADC1_IN0) | Analog temperature input |
| **Potentiometer** | Wiper (Pin 2) | STM32 **PA1** (ADC1_IN1) | Analog throttle/voltage input |
| **Common Ground** | STM32 **GND** | MCP2515, Lolin & Sensors| Mandatory common reference rail |
| **Power Distribution**| STM32 **3V3** | Breadboard (+) Rail | Regulated logic & sensor supply |

---

## 📊 Telemetry Frame Structure (`Telemetry_Packet_t`)
The system packs real-time sensor data into an efficient 4-byte structure transmitted over the CAN bus:

- Byte 0: `counter` (`uint8_t`) -> Rolling packet counter (0 to 255)
- Byte 1: `temperature` (`uint8_t`) -> LM35 converted temperature (°C)
- Byte 2: `potentiometer` (`uint8_t`) -> Scaled potentiometer / throttle input
- Byte 3: `checksum` (`uint8_t`) -> XOR-based packet integrity validation

---

## 🛠️ Environment & Toolchain Setup

* **MCU:** STM32F407VG (ARM Cortex-M4)
* **CAN Controller:** MCP2515 (8 MHz Crystal)
* **Wireless Node:** ESP8266 (Lolin / NodeMCU) running Wi-Fi AP Web Server
* **Toolchain:** STM32CubeIDE / GNU ARM Embedded Toolchain (`arm-none-eabi-gcc`)
* **Hardware Debugger:** ST-Link V2 (SWD) with real-time Live Expressions inspection

---

## 🚀 Roadmap / Next Steps
- [x] Implement interrupt-driven reception using the MCP2515 **INT** pin mapped to STM32 EXTI.
- [x] Build wireless serial bridge and web-based GCS dashboard (ESP8266).
- [ ] Switch from Loopback Mode to Normal Mode for multi-node physical bus communication.
- [ ] Integrate GPS module, HC-SR04 ultrasonic sensor, buzzer, and status LEDs.