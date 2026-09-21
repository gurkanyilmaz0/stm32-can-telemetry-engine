# STM32 Bare-Metal CAN & Wireless Telemetry Engine 🚀

A high-performance, register-level embedded telemetry engine implemented on an ARM Cortex-M4 (STM32F407VG) microcontroller. Built entirely without HAL (Hardware Abstraction Layer) or third-party middleware, this project integrates direct register manipulation for CAN communication, multi-channel ADC sensor acquisition (including internal MCU core temperature via ADC Channel 18), EXTI interrupts, UART GPS parsing with NMEA checksum validation, and a real-time wireless Ground Control Station (GCS) gateway via an ESP8266 (Lolin) node featuring browser-based reverse geocoding, link quality packet-loss metrics, mission uptime stopwatch, peak value tracking, and direct CSV log export.

---

## 🚀 Key Features

* **Pure Bare-Metal Architecture:** Zero HAL dependencies. Direct memory-mapped register manipulation for RCC, GPIO (MODER, AFRL), SPI1, ADC1, USART2, USART3, and EXTI.
* **Optimized MCP2515 CAN Integration:** Hardware-verified SPI communication configuring an external MCP2515 CAN controller with customized clock prescalers in loopback mode with custom filter bypass.
* **Multi-Channel & Internal ADC Acquisition:** Sequential sampling of analog sensors (LM35 Centigrade Temperature Sensor on PA0, Potentiometer on PA1) alongside the STM32 internal core temperature sensor (ADC Channel 18 with calibrated sample times).
* **Robust NMEA GPS Parser Engine:** Hardware UART3 ingestion of raw GPS data ($GPRMC$ / $GPGGA$ / $GSA$ sentences) protected by custom NMEA checksum validation, directly extracting latitude, longitude, UTC time, altitude, fix mode (2D/3D), and satellite counts.
* **Deterministic Packed Telemetry Framing:** Memory-aligned telemetry frames (`__attribute__((packed))`) containing rolling counter, sensor data, internal MCU temperature, and a custom XOR-based checksum (CRC).
* **Interrupt-Driven Reception:** External interrupt (`EXTI0` on `PB0`) triggered instantly upon packet arrival from the MCP2515 `INT` pin, alongside UART3 RXNE receive interrupts.
* **Advanced Tactical Wireless GCS Dashboard:** Real-time data bridging via UART2 to an ESP8266 (Lolin) acting as a Wi-Fi Access Point, rendering a professional dark-themed web dashboard equipped with live bar/gauge metrics, link quality packet-loss calculation, mission uptime stopwatch, peak (max) value tracking, threshold warning banners, dynamic OpenStreetMap reverse geocoding, and a one-click CSV flight log exporter.

---

## 📌 Hardware Pinout & Connections

| Component | Pin / Channel | Connected To | Description |
| :--- | :--- | :--- | :--- |
| **MCP2515 CS** | Chip Select | STM32 **PA4** (Output) | SPI1 Slave Select line |
| **SPI1 SCK** | Clock | STM32 **PA5** (AF5) | SPI Serial Clock |
| **SPI1 MISO** | Master In / Slave Out | STM32 **PA6** (AF5) | SPI Data Reception |
| **SPI1 MOSI** | Master Out / In | STM32 **PA7** (AF5) | SPI Data Transmission |
| **MCP2515 INT** | Interrupt | STM32 **PB0** (EXTI0) | Falling-edge packet arrival interrupt |
| **GPS Module** | TX -> RX | STM32 **PC10** (USART3_TX) / **PC11** (USART3_RX) | NMEA Telemetry Stream (9600 Baud) |
| **UART2 TX** | Transmit | ESP8266 (Lolin) **RX** | Wireless GCS bridge data stream (115200 Baud) |
| **LM35 Sensor** | Signal (Pin 2) | STM32 **PA0** (ADC1_IN0) | Analog temperature input |
| **Potentiometer** | Wiper (Pin 2) | STM32 **PA1** (ADC1_IN1) | Analog throttle/voltage input |
| **Common Ground** | STM32 **GND** | MCP2515, Lolin, GPS & Sensors| Mandatory common reference rail |
| **Power Distribution**| STM32 **3V3** | Breadboard (+) Rail | Regulated logic & sensor supply |

---

## 📊 Telemetry Frame Structure (`Telemetry_Packet_t`)
The system packs real-time sensor data into an efficient structure transmitted over the CAN bus, while concurrent string bridges format GPS coordinates and internal telemetry for the GCS:

- Byte 0: `counter` (`uint8_t`) -> Rolling packet counter (0 to 255)
- Byte 1: `temperature` (`uint8_t`) -> LM35 converted external temperature (°C)
- Byte 2: `mcu_temperature` (`uint8_t`) -> STM32 internal core temperature (°C)
- Byte 3: `potentiometer` (`uint8_t`) -> Scaled potentiometer / throttle input
- Byte 4: `checksum` (`uint8_t`) -> XOR-based packet integrity validation

---

## 🛠️ Environment & Toolchain Setup

* **MCU:** STM32F407VG (ARM Cortex-M4)
* **CAN Controller:** MCP2515 (8 MHz Crystal)
* **GPS Module:** NEO-M8N (NMEA protocol via UART3)
* **Wireless Node:** ESP8266 (Lolin / NodeMCU) running Wi-Fi AP Web Server, Live Metrics UI, and CSV Export
* **Toolchain:** STM32CubeIDE / GNU ARM Embedded Toolchain (`arm-none-eabi-gcc`)
* **Hardware Debugger:** ST-Link V2 (SWD) with real-time Live Expressions inspection

---

## 🚀 Roadmap / Next Steps
- [x] Implement interrupt-driven reception using the MCP2515 **INT** pin mapped to STM32 EXTI.
- [x] Integrate NEO-M8N GPS module with bare-metal UART3 NMEA parsing and checksum protection.
- [x] Build wireless serial bridge and tactical web-based GCS dashboard featuring link quality, uptime, peak trackers, and CSV logging (ESP8266).
- [ ] Switch from Loopback Mode to Normal Mode for multi-node physical bus communication.
- [ ] Integrate buzzer, status LEDs, and advanced flight/drive telemetry metrics.# STM32 Bare-Metal CAN & Wireless Telemetry Engine 🚀

A high-performance, register-level embedded telemetry engine implemented on an ARM Cortex-M4 (STM32F407VG) microcontroller. Built entirely without HAL (Hardware Abstraction Layer) or third-party middleware, this project integrates direct register manipulation for CAN communication, multi-channel ADC sensor acquisition (including internal MCU core temperature via ADC Channel 18), EXTI interrupts, UART GPS parsing with NMEA checksum validation, and a real-time wireless Ground Control Station (GCS) gateway via an ESP8266 (Lolin) node featuring browser-based reverse geocoding, link quality packet-loss metrics, mission uptime stopwatch, peak value tracking, and direct CSV log export.

---

## 🚀 Key Features

* **Pure Bare-Metal Architecture:** Zero HAL dependencies. Direct memory-mapped register manipulation for RCC, GPIO (MODER, AFRL), SPI1, ADC1, USART2, USART3, and EXTI.
* **Optimized MCP2515 CAN Integration:** Hardware-verified SPI communication configuring an external MCP2515 CAN controller with customized clock prescalers in loopback mode with custom filter bypass.
* **Multi-Channel & Internal ADC Acquisition:** Sequential sampling of analog sensors (LM35 Centigrade Temperature Sensor on PA0, Potentiometer on PA1) alongside the STM32 internal core temperature sensor (ADC Channel 18 with calibrated sample times).
* **Robust NMEA GPS Parser Engine:** Hardware UART3 ingestion of raw GPS data ($GPRMC$ / $GPGGA$ / $GSA$ sentences) protected by custom NMEA checksum validation, directly extracting latitude, longitude, UTC time, altitude, fix mode (2D/3D), and satellite counts.
* **Deterministic Packed Telemetry Framing:** Memory-aligned telemetry frames (`__attribute__((packed))`) containing rolling counter, sensor data, internal MCU temperature, and a custom XOR-based checksum (CRC).
* **Interrupt-Driven Reception:** External interrupt (`EXTI0` on `PB0`) triggered instantly upon packet arrival from the MCP2515 `INT` pin, alongside UART3 RXNE receive interrupts.
* **Advanced Tactical Wireless GCS Dashboard:** Real-time data bridging via UART2 to an ESP8266 (Lolin) acting as a Wi-Fi Access Point, rendering a professional dark-themed web dashboard equipped with live bar/gauge metrics, link quality packet-loss calculation, mission uptime stopwatch, peak (max) value tracking, threshold warning banners, dynamic OpenStreetMap reverse geocoding, and a one-click CSV flight log exporter.

---

## 📌 Hardware Pinout & Connections

| Component | Pin / Channel | Connected To | Description |
| :--- | :--- | :--- | :--- |
| **MCP2515 CS** | Chip Select | STM32 **PA4** (Output) | SPI1 Slave Select line |
| **SPI1 SCK** | Clock | STM32 **PA5** (AF5) | SPI Serial Clock |
| **SPI1 MISO** | Master In / Slave Out | STM32 **PA6** (AF5) | SPI Data Reception |
| **SPI1 MOSI** | Master Out / In | STM32 **PA7** (AF5) | SPI Data Transmission |
| **MCP2515 INT** | Interrupt | STM32 **PB0** (EXTI0) | Falling-edge packet arrival interrupt |
| **GPS Module** | TX -> RX | STM32 **PC10** (USART3_TX) / **PC11** (USART3_RX) | NMEA Telemetry Stream (9600 Baud) |
| **UART2 TX** | Transmit | ESP8266 (Lolin) **RX** | Wireless GCS bridge data stream (115200 Baud) |
| **LM35 Sensor** | Signal (Pin 2) | STM32 **PA0** (ADC1_IN0) | Analog temperature input |
| **Potentiometer** | Wiper (Pin 2) | STM32 **PA1** (ADC1_IN1) | Analog throttle/voltage input |
| **Common Ground** | STM32 **GND** | MCP2515, Lolin, GPS & Sensors| Mandatory common reference rail |
| **Power Distribution**| STM32 **3V3** | Breadboard (+) Rail | Regulated logic & sensor supply |

---

## 📊 Telemetry Frame Structure (`Telemetry_Packet_t`)
The system packs real-time sensor data into an efficient structure transmitted over the CAN bus, while concurrent string bridges format GPS coordinates and internal telemetry for the GCS:

- Byte 0: `counter` (`uint8_t`) -> Rolling packet counter (0 to 255)
- Byte 1: `temperature` (`uint8_t`) -> LM35 converted external temperature (°C)
- Byte 2: `mcu_temperature` (`uint8_t`) -> STM32 internal core temperature (°C)
- Byte 3: `potentiometer` (`uint8_t`) -> Scaled potentiometer / throttle input
- Byte 4: `checksum` (`uint8_t`) -> XOR-based packet integrity validation

---

## 🛠️ Environment & Toolchain Setup

* **MCU:** STM32F407VG (ARM Cortex-M4)
* **CAN Controller:** MCP2515 (8 MHz Crystal)
* **GPS Module:** NEO-M8N (NMEA protocol via UART3)
* **Wireless Node:** ESP8266 (Lolin / NodeMCU) running Wi-Fi AP Web Server, Live Metrics UI, and CSV Export
* **Toolchain:** STM32CubeIDE / GNU ARM Embedded Toolchain (`arm-none-eabi-gcc`)
* **Hardware Debugger:** ST-Link V2 (SWD) with real-time Live Expressions inspection

---

## 🚀 Roadmap / Next Steps
- [x] Implement interrupt-driven reception using the MCP2515 **INT** pin mapped to STM32 EXTI.
- [x] Integrate NEO-M8N GPS module with bare-metal UART3 NMEA parsing and checksum protection.
- [x] Build wireless serial bridge and tactical web-based GCS dashboard featuring link quality, uptime, peak trackers, and CSV logging (ESP8266).
- [ ] Switch from Loopback Mode to Normal Mode for multi-node physical bus communication.
- [ ] Integrate buzzer, status LEDs, and advanced flight/drive telemetry metrics.
