#include "can_driver.h"
#include <stdint.h>

/* --- STM32F407 Register Adres Tanımları --- */
#define RCC_BASE            (0x40023800UL)
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x40))

#define GPIOD_BASE          (0x40020C00UL)
#define GPIOD_MODER         (*(volatile uint32_t *)(GPIOD_BASE + 0x00))
#define GPIOD_OSPEEDR       (*(volatile uint32_t *)(GPIOD_BASE + 0x08))
#define GPIOD_AFR0          (*(volatile uint32_t *)(GPIOD_BASE + 0x20))

#define CAN1_BASE           (0x40006400UL)
#define CAN1_MCR            (*(volatile uint32_t *)(CAN1_BASE + 0x00))
#define CAN1_MSR            (*(volatile uint32_t *)(CAN1_BASE + 0x04))
#define CAN1_TSR            (*(volatile uint32_t *)(CAN1_BASE + 0x08))
#define CAN1_RF0R           (*(volatile uint32_t *)(CAN1_BASE + 0x0C))
#define CAN1_BTR            (*(volatile uint32_t *)(CAN1_BASE + 0x1C))

#define CAN1_TI0R           (*(volatile uint32_t *)(CAN1_BASE + 0x180))
#define CAN1_TDT0R          (*(volatile uint32_t *)(CAN1_BASE + 0x184))
#define CAN1_TDL0R          (*(volatile uint32_t *)(CAN1_BASE + 0x188))
#define CAN1_TDH0R          (*(volatile uint32_t *)(CAN1_BASE + 0x18C))

#define CAN1_RI0R           (*(volatile uint32_t *)(CAN1_BASE + 0x1B0))
#define CAN1_RDT0R          (*(volatile uint32_t *)(CAN1_BASE + 0x1B4))
#define CAN1_RDL0R          (*(volatile uint32_t *)(CAN1_BASE + 0x1B8))
#define CAN1_RDH0R          (*(volatile uint32_t *)(CAN1_BASE + 0x1BC))

#define CAN1_FMR            (*(volatile uint32_t *)(CAN1_BASE + 0x200))
#define CAN1_FM1R           (*(volatile uint32_t *)(CAN1_BASE + 0x204))
#define CAN1_FS1R           (*(volatile uint32_t *)(CAN1_BASE + 0x20C))
#define CAN1_FFA1R          (*(volatile uint32_t *)(CAN1_BASE + 0x214))
#define CAN1_FA1R           (*(volatile uint32_t *)(CAN1_BASE + 0x21C))
#define CAN1_F0R1           (*(volatile uint32_t *)(CAN1_BASE + 0x240))
#define CAN1_F0R2           (*(volatile uint32_t *)(CAN1_BASE + 0x244))

/* bxCAN Kesme Registerı & Cortex-M4 NVIC Adresleri */
#define CAN1_IER            (*(volatile uint32_t *)(CAN1_BASE + 0x014))
#define NVIC_ISER0          (*(volatile uint32_t *)(0xE000E100UL))
#define CAN1_RX0_IRQn       20

/* CAN1 Başlatma (Init) Fonksiyonu */
void CAN1_Init(CAN_BaudRate_t baudrate)
{
    /* 1. Clock Hatlarını Aç: GPIOD (AHB1 bit 3) ve CAN1 (APB1 bit 25) */
    RCC_AHB1ENR |= (1 << 3);
    RCC_APB1ENR |= (1 << 25);

    /* 2. GPIO Pinlerini Yapılandır: PD0 (RX) ve PD1 (TX) */
    /* Alternate function modu (MODER = 10) */
    GPIOD_MODER &= ~((3 << (0 * 2)) | (3 << (1 * 2)));
    GPIOD_MODER |=  ((2 << (0 * 2)) | (2 << (1 * 2)));

    /* Çok Yüksek Hız (Very High Speed - OSPEEDR = 11) */
    GPIOD_OSPEEDR |= ((3 << (0 * 2)) | (3 << (1 * 2)));

    /* Alternate Function 9 (CAN1) seçimi: AFR[0] bitleri */
    GPIOD_AFR0 &= ~((0xF << (0 * 4)) | (0xF << (1 * 4)));
    GPIOD_AFR0 |=  ((9 << (0 * 4))   | (9 << (1 * 4)));

    /* 3. bxCAN Başlatma Moduna Geçiş (INRQ = 1, SLEEP = 0) */
    CAN1_MCR &= ~(1 << 1); /* Sleep modundan çık */
    CAN1_MCR |= (1 << 0);  /* Init modunu talep et */
    while (!(CAN1_MSR & (1 << 0))); /* INAK bitini bekle */

    /* Otomatik Yeniden İletim (NART = 0: Hata olursa tekrar dene) */
    CAN1_MCR &= ~(1 << 4);

    /* 4. Baud Rate Hesaplaması (Varsayılan APB1 = 42 MHz) */
    /* Formül: Baud = APB1 / (Prescaler * (1 + TS1 + TS2)) */
    /* Prescaler = 6 için Toplam tq = 14: 1 + TS1(11) + TS2(2) = 14 tq -> 42MHz / (6 * 14) = 500 kbps */
    if (baudrate == CAN_BAUD_500K) {
        CAN1_BTR = (1 << 30) | (1 << 20) | (10 << 16) | (5 << 0); /* LBKM=1 (Loopback Modu Aktif) */
    } else if (baudrate == CAN_BAUD_250K) {
        CAN1_BTR = (0 << 30) | (1 << 20) | (10 << 16) | (11 << 0); /* Prescaler=12 -> 250 kbps */
    } else {
        CAN1_BTR = (0 << 30) | (1 << 20) | (10 << 16) | (2 << 0);  /* Prescaler=3 -> 1 Mbps */
    }

    /* 5. Normal Çalışma Moduna Geç (INRQ = 0) */
    CAN1_MCR &= ~(1 << 0);
    while (CAN1_MSR & (1 << 0)); /* Normal moda geçişi bekle */

    /* 6. Filtre Yapılandırması (Tüm standart ve extended paketleri kabul et) */
    CAN1_FMR |= (1 << 0);   /* Filtre başlatma modu (FINIT = 1) */
    CAN1_FA1R &= ~(1 << 0);  /* Filtre 0'ı devre dışı bırak */
    CAN1_FS1R |= (1 << 0);   /* Tekli 32-bit skala */
    CAN1_FM1R &= ~(1 << 0);  /* Maske modu */
    CAN1_F0R1 = 0x00000000;  /* ID: 0 */
    CAN1_F0R2 = 0x00000000;  /* Maske: 0 (Hepsini geçir) */
    CAN1_FFA1R &= ~(1 << 0); /* FIFO 0'a yönlendir */
    CAN1_FA1R |= (1 << 0);   /* Filtre 0'ı aktif et */
    CAN1_FMR &= ~(1 << 0);  /* Filtre başlatma modundan çık */
}

/* Mesaj Gönderme */
uint8_t CAN1_Transmit(const CAN_Message_t *txMsg)
{
    /* Mailbox 0 boş mu kontrol et (TME0 bit 26) */
    if (!(CAN1_TSR & (1 << 26))) {
        return 0; /* Mailbox dolu */
    }

    /* ID ve IDE yapılandırması */
    if (txMsg->ide == 0) {
        CAN1_TI0R = (txMsg->id << 21);
    } else {
        CAN1_TI0R = (txMsg->id << 3) | (1 << 2);
    }

    /* RTR biti */
    if (txMsg->rtr) {
        CAN1_TI0R |= (1 << 1);
    }

    /* Veri Uzunluğu (DLC) */
    CAN1_TDT0R = (txMsg->dlc & 0x0F);

    /* Veri Yükleme (Data Low ve Data High) */
    CAN1_TDL0R = ((uint32_t)txMsg->data[3] << 24) |
                 ((uint32_t)txMsg->data[2] << 16) |
                 ((uint32_t)txMsg->data[1] << 8)  |
                 ((uint32_t)txMsg->data[0]);

    CAN1_TDH0R = ((uint32_t)txMsg->data[7] << 24) |
                 ((uint32_t)txMsg->data[6] << 16) |
                 ((uint32_t)txMsg->data[5] << 8)  |
                 ((uint32_t)txMsg->data[4]);

    /* Gönderimi Başlat (TXRQ bit 0) */
    CAN1_TI0R |= (1 << 0);

    return 1;
}

/* Mesaj Okuma */
uint8_t CAN1_Receive(CAN_Message_t *rxMsg)
{
    /* FIFO 0'da mesaj var mı? (FMP0 bits 1:0) */
    if ((CAN1_RF0R & 0x03) == 0) {
        return 0; /* Mesaj yok */
    }

    /* IDE kontrolü */
    if (CAN1_RI0R & (1 << 2)) {
        rxMsg->ide = 1;
        rxMsg->id = (CAN1_RI0R >> 3);
    } else {
        rxMsg->ide = 0;
        rxMsg->id = (CAN1_RI0R >> 21);
    }

    rxMsg->rtr = (CAN1_RI0R & (1 << 1)) ? 1 : 0;
    rxMsg->dlc = (CAN1_RDT0R & 0x0F);

    /* Verileri Oku */
    uint32_t dataLow = CAN1_RDL0R;
    uint32_t dataHigh = CAN1_RDH0R;

    rxMsg->data[0] = (uint8_t)(dataLow & 0xFF);
    rxMsg->data[1] = (uint8_t)((dataLow >> 8) & 0xFF);
    rxMsg->data[2] = (uint8_t)((dataLow >> 16) & 0xFF);
    rxMsg->data[3] = (uint8_t)((dataLow >> 24) & 0xFF);

    rxMsg->data[4] = (uint8_t)(dataHigh & 0xFF);
    rxMsg->data[5] = (uint8_t)((dataHigh >> 8) & 0xFF);
    rxMsg->data[6] = (uint8_t)((dataHigh >> 16) & 0xFF);
    rxMsg->data[7] = (uint8_t)((dataHigh >> 24) & 0xFF);

    /* Mesajı FIFO 0'dan serbest bırak (RFOM0 bit 5) */
    CAN1_RF0R |= (1 << 5);

    return 1;
}

/* CAN1 RX FIFO0 Kesmesini (FMPIE0) ve Cortex-M4 NVIC Hattını Etkinleştir */
void CAN1_Enable_RX_Interrupt(void)
{
    /* 1. bxCAN FIFO 0 Message Pending Kesmesini Etkinleştir (FMPIE0 = Bit 1) */
    CAN1_IER |= (1 << 1);

    /* 2. Cortex-M4 NVIC Hattını Aç (IRQ 20) */
    NVIC_ISER0 |= (1 << CAN1_RX0_IRQn);
}
