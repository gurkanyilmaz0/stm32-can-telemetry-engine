#include <stdint.h>

#define RCC_BASE        (0x40023800UL)
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x44UL))

#define GPIOA_BASE      (0x40020000UL)
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_ODR       (*(volatile uint32_t *)(GPIOA_BASE + 0x14UL))
#define GPIOA_AFRL      (*(volatile uint32_t *)(GPIOA_BASE + 0x20UL))

#define SPI1_BASE       (0x40013000UL)
#define SPI1_CR1        (*(volatile uint32_t *)(SPI1_BASE + 0x00UL))
#define SPI1_SR         (*(volatile uint32_t *)(SPI1_BASE + 0x08UL))
#define SPI1_DR         (*(volatile uint32_t *)(SPI1_BASE + 0x0CUL))

// ADC1 Register Tanımları
#define ADC1_BASE       (0x40012000UL)
#define ADC1_SR         (*(volatile uint32_t *)(ADC1_BASE + 0x00UL))
#define ADC1_CR2        (*(volatile uint32_t *)(ADC1_BASE + 0x08UL))
#define ADC1_SQR3       (*(volatile uint32_t *)(ADC1_BASE + 0x34UL))
#define ADC1_DR         (*(volatile uint32_t *)(ADC1_BASE + 0x4CUL))

// MCP2515 Komutları ve Register Adresleri
#define MCP_RESET       0xC0
#define MCP_WRITE       0x02
#define MCP_READ        0x03
#define MCP_CANSTAT     0x0E
#define MCP_CANCTRL     0x0F
#define MCP_CANINTF     0x2C

#define MCP_CNF1        0x2A
#define MCP_CNF2        0x29
#define MCP_CNF3        0x28

#define MCP_TXB0SIDH    0x31
#define MCP_RTS_TX0     0x81

// MCP2515 Çalışma Modları
#define MCP_MODE_NORMAL     0x00
#define MCP_MODE_LOOPBACK   0x40
#define MCP_MODE_CONFIG     0x80

// --- PROFESYONEL TELEMETRİ STRUCT MİMARİSİ ---
typedef struct __attribute__((packed)) {
    uint8_t counter;
    uint8_t temperature;
    uint8_t potentiometer;
} Telemetry_Packet_t;

// --- GLOBAL DEĞİŞKENLER ---
volatile uint8_t debug_val = 0;
volatile uint8_t raw_spi_test = 0;
volatile uint8_t packet_counter = 0;

volatile Telemetry_Packet_t tx_packet = {0};
volatile Telemetry_Packet_t rx_packet = {0};

volatile uint16_t pot_voltage = 0;
volatile uint8_t lm35_temp_c = 0;

void SystemInit(void) {}

void delay_ms(uint32_t ms) {
    volatile uint32_t count = ms * 1600;
    while (count--) __asm__("NOP");
}

uint16_t ADC_Read(uint8_t channel) {
    ADC1_SQR3 = channel;
    ADC1_CR2 |= (1UL << 30);
    while (!(ADC1_SR & (1UL << 1)));
    return (uint16_t)ADC1_DR;
}

uint8_t SPI1_TransmitReceive(uint8_t byte) {
    uint32_t timeout = 0x5000;
    while (!(SPI1_SR & (1UL << 1))) {
        if (--timeout == 0) return 0xEE;
    }

    *(volatile uint8_t *)&SPI1_DR = byte;

    timeout = 0x5000;
    while (!(SPI1_SR & (1UL << 0))) {
        if (--timeout == 0) return 0xAA;
    }

    return *(volatile uint8_t *)&SPI1_DR;
}

void MCP2515_Select(void) { GPIOA_ODR &= ~(1UL << 4); }
void MCP2515_Deselect(void) { GPIOA_ODR |= (1UL << 4); }

void MCP2515_Reset(void) {
    MCP2515_Select();
    SPI1_TransmitReceive(MCP_RESET);
    MCP2515_Deselect();
    delay_ms(10);
}

uint8_t MCP2515_ReadRegister(uint8_t address) {
    uint8_t data;
    MCP2515_Select();
    SPI1_TransmitReceive(MCP_READ);
    SPI1_TransmitReceive(address);
    data = SPI1_TransmitReceive(0x00);
    MCP2515_Deselect();
    return data;
}

void MCP2515_WriteRegister(uint8_t address, uint8_t data) {
    MCP2515_Select();
    SPI1_TransmitReceive(MCP_WRITE);
    SPI1_TransmitReceive(address);
    SPI1_TransmitReceive(data);
    MCP2515_Deselect();
}

// CAN Paketi Gönderme Fonksiyonu
void MCP2515_SendPacket(volatile Telemetry_Packet_t *pkt) {
    MCP2515_Select();
    SPI1_TransmitReceive(MCP_WRITE);
    SPI1_TransmitReceive(MCP_TXB0SIDH);

    SPI1_TransmitReceive(0x03); // ID High
    SPI1_TransmitReceive(0x60); // ID Low
    SPI1_TransmitReceive(0x00); // EXID High
    SPI1_TransmitReceive(0x00); // EXID Low
    SPI1_TransmitReceive(0x03); // DLC (3 Bayt veri)

    SPI1_TransmitReceive(pkt->counter);
    SPI1_TransmitReceive(pkt->temperature);
    SPI1_TransmitReceive(pkt->potentiometer);
    MCP2515_Deselect();

    // RTS Tetikle
    MCP2515_Select();
    SPI1_TransmitReceive(MCP_RTS_TX0);
    MCP2515_Deselect();
}

// CAN Paketi Okuma Fonksiyonu (RX Buffer 0 Doğrudan Veri Başlangıcı: 0x92)
void MCP2515_ReceivePacket(volatile Telemetry_Packet_t *rx_pkt) {
    MCP2515_Select();
    SPI1_TransmitReceive(0x92); // Doğrudan RXB0 Veri Alanından Okumaya Başla

    rx_pkt->counter       = SPI1_TransmitReceive(0x00);
    rx_pkt->temperature   = SPI1_TransmitReceive(0x00);
    rx_pkt->potentiometer = SPI1_TransmitReceive(0x00);

    MCP2515_Deselect();
}

int main(void) {
    // 1. Clock Yapılandırması
    RCC_AHB1ENR |= (1UL << 0);
    RCC_APB2ENR |= (1UL << 8);
    RCC_APB2ENR |= (1UL << 12);

    // PA0 ve PA1 Analog Mod
    GPIOA_MODER |= (3UL << (0 * 2)) | (3UL << (1 * 2));

    // SPI1 Pinleri (PA4 CS Output, PA5 SCK AF5, PA6 MISO AF5, PA7 MOSI AF5)
    GPIOA_MODER &= ~((3UL << (4 * 2)) | (3UL << (5 * 2)) | (3UL << (6 * 2)) | (3UL << (7 * 2)));
    GPIOA_MODER |=  ((1UL << (4 * 2)) | (2UL << (5 * 2)) | (2UL << (6 * 2)) | (2UL << (7 * 2)));

    GPIOA_AFRL &= ~((0xFUL << (5 * 4)) | (0xFUL << (6 * 4)) | (0xFUL << (7 * 4)));
    GPIOA_AFRL |=  ((5UL << (5 * 4)) | (5UL << (6 * 4)) | (5UL << (7 * 4)));

    // SPI1 Yapılandırması: Master, fPCLK/8, Yazılımsal NSS (SSM=1, SSI=1), SPE=1
    SPI1_CR1 = (1UL << 2) | (1UL << 3) | (1UL << 6) | (1UL << 9) | (1UL << 8);

    // ADC1 On
    ADC1_CR2 |= (1UL << 0);
    delay_ms(10);

    MCP2515_Deselect();
    delay_ms(50);

    // --- MCP2515 BAŞLANGIÇ VE MOD AYARI ---
    MCP2515_Reset();

    // 8MHz Kristal için 500 kbps Baudrate Ayarları
    MCP2515_WriteRegister(MCP_CNF1, 0x00);
    MCP2515_WriteRegister(MCP_CNF2, 0x90);
    MCP2515_WriteRegister(MCP_CNF3, 0x02);

    // Loopback Moduna Al
    MCP2515_WriteRegister(MCP_CANCTRL, MCP_MODE_LOOPBACK);

    raw_spi_test = MCP2515_ReadRegister(MCP_CANSTAT);

    while (1) {
        debug_val = MCP2515_ReadRegister(MCP_CANSTAT);

        // ADC Okumaları
        uint16_t adc_lm35 = ADC_Read(0);
        lm35_temp_c = (uint8_t)(adc_lm35 * 330 / 4095);
        pot_voltage = ADC_Read(1);

        // Gönderim Paketini Güncelle
        tx_packet.counter = packet_counter;
        tx_packet.temperature = lm35_temp_c;
        tx_packet.potentiometer = (uint8_t)(pot_voltage >> 4);

        // 1. Paketi CAN Üzerinden Fırlat
        MCP2515_SendPacket(&tx_packet);

        delay_ms(10); // Donanımın döngüyü işlemesi için minik bir nefes

        // 2. Loopback Üzerinden Geri Dönen Paketi Oku
        MCP2515_ReceivePacket(&rx_packet);

        packet_counter++;
        delay_ms(500);
    }
}
