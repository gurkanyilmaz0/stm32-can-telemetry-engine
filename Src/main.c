#include <stdint.h>
#include "can_driver.h"
#include "telemetry.h"

/* CAN1 RX0 Donanım Kesme Rutini (ISR) */
void CAN1_RX0_IRQHandler(void)
{
    /* FIFO 0'a paket düştüğü an CPU otomatik buraya sıçrar */
    Telemetry_ProcessRx();
}

static void delay_cycles(volatile uint32_t count)
{
    while (count--) {
        __asm("nop");
    }
}

int main(void)
{
    /* 1. Donanım Başlatma */
    CAN1_Init(CAN_BAUD_500K);

    /* 2. CAN RX0 Donanım Kesmesini (NVIC + bxCAN) Devreye Al */
    CAN1_Enable_RX_Interrupt();

    /* 3. Uygulama Katmanı Başlatma */
    Telemetry_Init();

    while (1)
    {
        /* main döngüsü yalnızca periyodik iletim yapar; alım kesmeyle asenkron gerçekleşir */
        Telemetry_ProcessTx();

        /* ~100 ms Telemetri Periyodu */
        delay_cycles(160000);
    }
}
