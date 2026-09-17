Kusura bakma kanka, ben komutları kastettin sandım!

Eski CAN bus mimarisini, bit timing hesaplamalarını, frame tablolarını ve yeni eklediğimiz LM35, Potansiyometre ve Lolin (ESP8266) kablosuz telemetri sistemini bir araya getiren **eksiksiz ve birleşik README metninin tamamını** tek bir siyah kod kutusunda veriyorum.

Kutunun sağ üstündeki kopyala butonuna basıp doğrudan `README.md` dosyasının içine yapıştırabilirsin:

```markdown
# STM32 Bare-Metal CAN & Wireless Telemetry Engine

A high-performance, register-level embedded telemetry engine implemented on an ARM Cortex-M4 (STM32F407VG) microcontroller. Built entirely without HAL (Hardware Abstraction Layer) or third-party middleware, this project integrates direct register manipulation for CAN communication, multi-channel ADC sensor acquisition, and a real-time wireless gateway bridge via an ESP8266 (Lolin) node.

---

## 🚀 Key Features

* **Pure Bare-Metal Architecture:** Zero HAL dependencies. Direct memory-mapped register manipulation for RCC, GPIO, bxCAN, ADC1, USART1, and Cortex-M4 NVIC.
* **Optimized bxCAN Bit Timing:** Engineered for 500 kbps on a 42 MHz APB1 peripheral clock, maintaining an 85.7% sample point.
* **Multi-Channel ADC Acquisition:** Sequential sampling of analog sensors (LM35 Centigrade Temperature Sensor on PA0, Potentiometer on PA1).
* **Wireless Gateway Bridge:** Real-time UART bridge connecting the STM32 core transmitter to an ESP8266 (Lolin) Wi-Fi node.
* **Clock-Drift Compensation:** Compensated for internal HSI RC oscillator drift by tuning UART transmission to 1200 Baud for zero packet loss.
* **Deterministic Big-Endian Framing:** 8-byte packed CAN telemetry frames with custom scaling factors and integrity verification markers.
* **Interrupt-Driven Asynchronous RX:** Non-blocking FIFO 0 message reception via `CAN1_RX0_IRQHandler` mapped to Cortex-M4 NVIC IRQ 20.
* **Hardware Self-Test Verification:** Integrated internal loopback mode validation observable via ST-Link Live Expressions.

---

## 📌 Hardware Pinout & Connections

| Component | Pin / Channel | Connected To | Description |
| :--- | :--- | :--- | :--- |
| **LM35 Sensor** | Signal (Pin 2) | STM32 **PA0** (ADC1_IN0) | Analog temperature input (10 mV/°C) |
| **Potentiometer** | Wiper (Pin 2) | STM32 **PA1** (ADC1_IN1) | Analog throttle/voltage input (0 - 3.3V) |
| **STM32 UART TX** | STM32 **PA9** | Lolin **D1** (GPIO5 / RX) | Telemetry serial data link |
| **CAN RX (Bus)** | STM32 **PD0** | CAN Transceiver RX | High-speed differential bus reception |
| **CAN TX (Bus)** | STM32 **PD1** | CAN Transceiver TX | High-speed differential bus transmission |
| **Common Ground** | STM32 **GND** | Lolin **GND** & Sensors | Mandatory common reference rail |
| **Power Distribution**| STM32 **3V3** | Breadboard (+) Rail | Regulated logic & sensor supply |

---

## 📂 Architectural Overview

```text
stm32-can-telemetry-engine/
├── App/                         # Application Layer
│   ├── telemetry.h              # Telemetry payload definitions & API
│   └── telemetry.c              # Serialization, parsing & state handlers
├── Drivers_Custom/              # Hardware Driver Layer
│   └── can_driver.c             # Direct register configuration for bxCAN & NVIC
├── Inc/                         # Core Headers
│   └── can_driver.h             # Peripheral registers & frame structures
├── Src/                         # System Entry Point
│   └── main.c                   # Super-loop scheduler, ADC sampling & UART telemetry
└── gateway/                     # Wireless Gateway
    └── lolin_gateway.ino        # ESP8266 SoftwareSerial wireless bridge firmware
```

---

## ⏱️ CAN Bit Timing & Clock Calculations

Operating on an APB1 bus clock of 42 MHz, the bit timing is configured for 500 kbps using 14 Time Quanta ($t_q$) per nominal bit:

$$\text{Bit Rate} = \frac{f_{\text{APB1}}}{\text{BRP} \times (1 + \text{TS1} + \text{TS2})} = \frac{42\text{ MHz}}{6 \times 14} = 500\text{ kbps}$$

| Parameter | Configuration Value | Register Bitfield | Description |
| :--- | :--- | :--- | :--- |
| **Prescaler (BRP)** | 6 | `BRP = 5` | Prescaler divider ($t_q = 142.85\text{ ns}$) |
| **Sync Segment** | 1 $t_q$ | Fixed | Bit synchronization |
| **Time Segment 1 (TS1)** | 11 $t_q$ | `TS1 = 10` | Propagation & Phase Segment 1 |
| **Time Segment 2 (TS2)** | 2 $t_q$ | `TS2 = 1` | Phase Segment 2 |
| **Sample Point** | 85.7% | - | $(1 + 11) / 14 = 85.7\%$ (Optimized for automotive buses) |
| **Resynchronization Jump (SJW)** | 1 $t_q$ | `SJW = 0` | Resynchronization limit |

---

## 📊 Telemetry Frame Structures

### 1. High-Speed CAN Frame (Identifier: `0x150`, 8 Bytes)

| Byte Index | Field Name | Data Type | Endianness | Scaling / Resolution | Unit |
| :---: | :--- | :---: | :---: | :---: | :---: |
| **Byte 0 - 1** | `pack_voltage_raw` | `uint16_t` | Big-Endian (MSB First) | 0.01 V ($4850 = 48.50\text{ V}$) | V |
| **Byte 2 - 3** | `motor_temp_raw` | `uint16_t` | Big-Endian (MSB First) | 0.1 °C ($350 = 35.0\text{ °C}$) | °C |
| **Byte 4** | `system_state` | `uint8_t` | - | 0: Normal, 1: Warning, 2: Error | Enum |
| **Byte 5** | `error_code` | `uint8_t` | - | Bitwise fault indicators | Bitfield |
| **Byte 6** | `counter` | `uint8_t` | - | 0 → 255 Rolling packet counter | Count |
| **Byte 7** | `frame_marker` | `uint8_t` | - | `0xAA` (Frame tail / integrity marker) | Constant |

### 2. Wireless UART Telemetry Stream
The STM32 periodically streams human-readable and gateway-parseable payloads over USART1:
```text
POT:3.29V | TEMP:34.43C
```

---

## ⚡ Interrupt & Reception Pipeline

Instead of blocking the super-loop with FIFO polling, CAN reception is driven asynchronously by hardware:

1. **bxCAN Filter Bank:** Filter 0 is configured in 32-bit identifier mask mode, accepting all standard frames and routing them to FIFO 0.
2. **Interrupt Trigger:** Hardware asserts `FMPIE0` (FIFO 0 Message Pending Interrupt).
3. **Core Vectoring:** Cortex-M4 NVIC IRQ 20 handles the vector and executes `CAN1_RX0_IRQHandler()`.
4. **Data Extraction:** Direct register extraction reads `CAN1_RDL0R` and `CAN1_RDH0R`, releases the mailbox via `RFOM0`, and parses data into the telemetry struct.

---

## 🛠️ Hardware & Environment Setup

* **MCU:** STM32F407VG (ARM Cortex-M4 @ 168 MHz)
* **Wireless Node:** Lolin NodeMCU (ESP8266 12-E)
* **Toolchain:** STM32CubeIDE / GNU ARM Embedded Toolchain (`arm-none-eabi-gcc`)
* **Hardware Debugger:** ST-Link V2 (SWD) with real-time Live Expressions inspection

```