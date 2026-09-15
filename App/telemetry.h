#ifndef TELEMETRY_H_
#define TELEMETRY_H_

#include <stdint.h>
#include "can_driver.h"

/* Telemetri CAN Mesaj Tanımları */
#define TELEMETRY_CAN_ID_STATUS    0x150
#define TELEMETRY_PAYLOAD_SIZE     8

/* Telemetri Sensör ve Sistem Yapısı */
typedef struct {
    uint16_t pack_voltage_raw; /* 0.01V çözünürlük (Örn: 4850 -> 48.50V) */
    uint16_t motor_temp_raw;   /* 0.1C çözünürlük  (Örn: 350  -> 35.0C)  */
    uint8_t  system_state;     /* 0: Normal, 1: Uyarı, 2: Hata */
    uint8_t  error_code;       /* Sistem hata kodu */
    uint8_t  counter;          /* Paket sayacı (0-255) */
} Telemetry_Data_t;

/* Global olarak gözlemlenebilir nesneler */
extern volatile Telemetry_Data_t g_telemetry_tx;
extern volatile Telemetry_Data_t g_telemetry_rx;

/* Fonksiyon Prototipleri */
void Telemetry_Init(void);
void Telemetry_ProcessTx(void);
uint8_t Telemetry_ProcessRx(void);

#endif /* TELEMETRY_H_ */
