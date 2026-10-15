# STM32F407 Bare-Metal Telemetry & CAN Engine 🚀

Register-level (HAL-free) telemetry system on an STM32F407VG (ARM Cortex-M4). The MCU reads onboard/external sensors, parses live GPS data over UART, frames the data into a CAN packet via an MCP2515 controller, and bridges the same telemetry wirelessly to a browser-based ground station dashboard through an ESP8266 access point.

---

## ✅ Completed Features

| Feature | Status | Notes |
|---|---|---|
| Bare-metal peripheral init (RCC, GPIO, AFRL/AFRH) | ✅ Done | No HAL/CMSIS, direct register access |
| SPI1 master driver | ✅ Done | Polling-based, timeout-guarded |
| MCP2515 CAN controller driver | ✅ Done | Reset, config, register R/W, TX via `RTS` |
| CAN loopback self-test | ✅ Done | Verified TX path in `MODE_LOOPBACK` |
| EXTI interrupt from MCP2515 INT pin | ✅ Done | Reads `EFLG` register on falling edge |
| Multi-channel ADC sampling | ✅ Done | LM35 (PA0), potentiometer (PA1), internal core temp (Ch18) |
| HC-SR04 ultrasonic distance sensor | ✅ Done | Trigger/echo timing with timeout guard |
| UART3 interrupt-driven GPS reception | ✅ Done | RXNE ISR, line-buffered |
| NMEA sentence parsing (GPRMC/GPGGA/GSA) | ✅ Done | Manual field tokenizing, no external lib |
| NMEA checksum validation | ✅ Done | XOR checksum against `*hh` field |
| Packed telemetry struct + XOR checksum | ✅ Done | `__attribute__((packed))`, 6-byte frame |
| Local telemetry history buffer | ✅ Done | 50-entry circular buffer in RAM |
| UART2 bridge to ESP8266 | ✅ Done | Formatted ASCII string, 115200 baud |
| Threshold-based alarm (LED + buzzer) | ✅ Done | Temp > 35°C or distance < 30cm |
| ESP8266 Wi-Fi AP + web dashboard | ✅ Done | Self-hosted, no external CDN dependency |
| Live canvas-based telemetry graph | ✅ Done | Pure JS, no external chart library |
| Packet-loss / link quality estimation | ✅ Done | Rolling counter gap detection |
| CSV flight log export (client-side) | ✅ Done | Browser `Blob` download |
| OTA firmware update (ESP8266 side) | ✅ Done | `ArduinoOTA` |

---

## 📌 Hardware Pinout & Connections

| Component | Pin / Channel | Connected To | Description |
| :--- | :--- | :--- | :--- |
| **MCP2515 CS** | Chip Select | STM32 **PA4** (Output) | SPI1 Slave Select line |
| **SPI1 SCK** | Clock | STM32 **PA5** (AF5) | SPI Serial Clock |
| **SPI1 MISO** | Master In / Slave Out | STM32 **PA6** (AF5) | SPI Data Reception |
| **SPI1 MOSI** | Master Out / In | STM32 **PA7** (AF5) | SPI Data Transmission |
| **MCP2515 INT** | Interrupt | STM32 **PB0** (EXTI0) | Falling-edge packet arrival interrupt |
| **GPS Module** | TX → RX | STM32 **PC10** (USART3_TX) / **PC11** (USART3_RX) | NMEA stream, 9600 baud |
| **UART2 TX** | Transmit | ESP8266 (Lolin) **RX** | Wireless bridge, 115200 baud |
| **LM35 Sensor** | Signal (Pin 2) | STM32 **PA0** (ADC1_IN0) | Analog external temperature |
| **Potentiometer** | Wiper (Pin 2) | STM32 **PA1** (ADC1_IN1) | Analog throttle/reference input |
| **HC-SR04 Trig** | Trigger | STM32 **PB6** (Output) | Ultrasonic pulse trigger |
| **HC-SR04 Echo** | Echo | STM32 **PB7** (Input) | Ultrasonic echo timing |
| **Status LED (Green)** | Output | STM32 **PC0** | System running indicator |
| **Status LED (Red)** | Output | STM32 **PC1** | Alarm indicator |
| **Buzzer** | Output | STM32 **PB8** | Alarm audio output |
| **Common Ground** | GND | MCP2515, ESP8266, GPS, sensors | Shared reference rail |
| **Power Distribution** | 3V3 | Breadboard (+) rail | Regulated logic/sensor supply |

---

## 📊 Telemetry Frame Structure (`Telemetry_Packet_t`)

| Byte | Field | Type | Description |
|---|---|---|---|
| 0 | `counter` | `uint8_t` | Rolling packet counter (0–255) |
| 1 | `temperature` | `uint8_t` | LM35 external temperature (°C) |
| 2 | `mcu_temperature` | `uint8_t` | STM32 internal core temperature (°C) |
| 3 | `potentiometer` | `uint8_t` | Scaled potentiometer reading |
| 4 | `distance` | `uint8_t` | HC-SR04 distance (cm) |
| 5 | `checksum` | `uint8_t` | XOR of bytes 0–4 |

---

## 🛠️ Environment & Toolchain

| Component | Detail |
|---|---|
| MCU | STM32F407VG (ARM Cortex-M4) |
| CAN Controller | MCP2515 (8 MHz crystal) |
| GPS Module | NEO-M8N (NMEA, UART) |
| Wireless Node | ESP8266 (Lolin/NodeMCU) |
| Toolchain | STM32CubeIDE / `arm-none-eabi-gcc` |
| Debugger | ST-Link V2 (SWD) |
| Dashboard Frontend | Vanilla HTML/CSS/JS, canvas-based charting (no CDN) |
