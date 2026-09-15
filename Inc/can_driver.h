#ifndef INC_CAN_DRIVER_H_
#define INC_CAN_DRIVER_H_

#include <stdint.h>

/* CAN Mesaj Çerçevesi (Frame) Yapısı */
typedef struct {
    uint32_t id;       /* Standart (11-bit) veya Genişletilmiş (29-bit) ID */
    uint8_t  ide;      /* 0: Standart ID (11-bit), 1: Extended ID (29-bit) */
    uint8_t  rtr;      /* 0: Data Frame, 1: Remote Frame */
    uint8_t  dlc;      /* Veri Uzunluğu (0 - 8 byte) */
    uint8_t  data[8];  /* Veri Alanı */
} CAN_Message_t;

/* CAN Baud Rate Seçenekleri (APB1 = 42 MHz varsayılarak) */
typedef enum {
    CAN_BAUD_250K = 0,
    CAN_BAUD_500K,
    CAN_BAUD_1M
} CAN_BaudRate_t;

/* Sürücü Fonksiyon Prototipleri */
void CAN1_Init(CAN_BaudRate_t baudrate);
uint8_t CAN1_Transmit(const CAN_Message_t *txMsg);
uint8_t CAN1_Receive(CAN_Message_t *rxMsg);
void CAN1_Enable_RX_Interrupt(void);

#endif /* INC_CAN_DRIVER_H_ */
