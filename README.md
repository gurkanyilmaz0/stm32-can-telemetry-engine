# STM32 Bare-Metal CAN Telemetry Engine

A high-performance, register-level CAN (Controller Area Network) telemetry engine implemented on an ARM Cortex-M4 (STM32F407VG) microcontroller. Built entirely without HAL (Hardware Abstraction Layer) or third-party middleware, this project demonstrates direct register manipulation, modular layered architecture, deterministic big-endian packet serialization, and an asynchronous, interrupt-driven RX pipeline.

---

## Key Features

* **Pure Bare-Metal Implementation:** Zero HAL dependencies. Direct memory-mapped register manipulation for RCC, GPIOD (AF9), bxCAN, and Cortex-M4 NVIC.
* **Optimized bxCAN Bit Timing:** Engineered for 500 kbps on a 42 MHz APB1 peripheral clock, maintaining an 85.7% sample point.
* **Layered Software Architecture:** Complete decoupling between physical peripheral access (Drivers_Custom/) and application-level telemetry parsing (App/).
* **Deterministic Big-Endian Framing:** 8-byte packed telemetry frames with custom scaling factors and frame-tail marker verification.
* **Interrupt-Driven Asynchronous RX:** Non-blocking FIFO 0 message reception via CAN1_RX0_IRQHandler mapped to Cortex-M4 NVIC IRQ 20.
* **Hardware Self-Test Verification:** Integrated internal loopback mode validation observable via ST-Link Live Expressions.

---

## Architectural Overview

stm32-can-telemetry-engine/
├── App/                         # Application Layer
│   ├── telemetry.h              # Telemetry payload definitions & API
│   └── telemetry.c              # Serialization, parsing & state handlers
├── Drivers_Custom/              # Hardware Abstraction / Driver Layer
│   └── can_driver.c             # Direct register configuration for bxCAN & NVIC
├── Inc/                         # Core Headers
│   └── can_driver.h             # Peripheral registers & CAN frame structures
└── Src/                         # System Entry Point
    └── main.c                   # Super-loop scheduler & ISR entry point

---

## CAN Bit Timing & Clock Calculations

Operating on an APB1 bus clock of 42 MHz, the bit timing is configured for 500 kbps using 14 Time Quanta (tq) per nominal bit:

Bit Rate = APB1 / (Prescaler * (1 + TS1 + TS2)) = 42 MHz / (6 * 14) = 500 kbps

| Parameter | Configuration Value | Register Bitfield | Description |
| :--- | :--- | :--- | :--- |
| Prescaler (BRP) | 6 | BRP = 5 | Prescaler divider (tq = 142.85 ns) |
| Sync Segment | 1 tq | Fixed | Bit synchronization |
| Time Segment 1 (TS1) | 11 tq | TS1 = 10 | Propagation & Phase Segment 1 |
| Time Segment 2 (TS2) | 2 tq | TS2 = 1 | Phase Segment 2 |
| Sample Point | 85.7% | - | (1 + 11) / 14 = 85.7% (Ideal for automotive/industrial buses) |
| Resynchronization Jump Width (SJW) | 1 tq | SJW = 0 | Resynchronization limit |

---

## Telemetry Frame Structure

The status frame uses standard 11-bit identifiers (ID = 0x150) with an 8-byte payload:

| Byte Index | Field Name | Data Type | Endianness | Scaling / Resolution | Unit |
| :---: | :--- | :---: | :---: | :---: | :---: |
| Byte 0 - 1 | pack_voltage_raw | uint16_t | Big-Endian (MSB First) | 0.01 V (4850 = 48.50 V) | V |
| Byte 2 - 3 | motor_temp_raw | uint16_t | Big-Endian (MSB First) | 0.1 C (350 = 35.0 C) | C |
| Byte 4 | system_state | uint8_t | - | 0: Normal, 1: Warning, 2: Error | Enum |
| Byte 5 | error_code | uint8_t | - | Bitwise fault indicators | Bitfield |
| Byte 6 | counter | uint8_t | - | 0 -> 255 Rolling packet counter | Count |
| Byte 7 | frame_marker | uint8_t | - | 0xAA (Frame tail / integrity marker) | Constant |

---

## Interrupt & Reception Pipeline

Instead of blocking the super-loop with FIFO polling, reception is driven asynchronously by hardware:

1. bxCAN Filter Bank: Filter 0 configured in 32-bit mask mode, accepting all frames and routing to FIFO 0.
2. Interrupt Trigger: bxCAN raises FMPIE0 (FIFO 0 Message Pending Interrupt).
3. Core Vectoring: Cortex-M4 NVIC IRQ 20 handles the vector and executes CAN1_RX0_IRQHandler().
4. Data Extraction: Raw registers (CAN1_RDL0R, CAN1_RDH0R) are read, the message is released via RFOM0, and unpacked into g_telemetry_rx.

---

## Hardware & Environment Setup

* MCU: STM32F407VG (ARM Cortex-M4 @ 168 MHz)
* CAN Transceiver Pins:
  * PD0 -> CAN1_RX (AF9)
  * PD1 -> CAN1_TX (AF9)
* IDE & Toolchain: STM32CubeIDE / GNU ARM Embedded Toolchain (arm-none-eabi-gcc)
* Debugger: ST-Link V2 (SWD) with real-time Live Expressions inspection