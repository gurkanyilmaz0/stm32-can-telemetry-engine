#include <stdint.h>
#include <stdio.h>

#define RCC_BASE        (0x40023800UL)
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x40UL))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x44UL))

#define GPIOA_BASE      (0x40020000UL)
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_ODR       (*(volatile uint32_t *)(GPIOA_BASE + 0x14UL))
#define GPIOA_AFRL      (*(volatile uint32_t *)(GPIOA_BASE + 0x20UL))

#define GPIOB_BASE      (0x40020400UL)
#define GPIOB_MODER     (*(volatile uint32_t *)(GPIOB_BASE + 0x00UL))

#define SYSCFG_BASE     (0x40013800UL)
#define SYSCFG_EXTICR1  (*(volatile uint32_t *)(SYSCFG_BASE + 0x08UL))

#define EXTI_BASE       (0x40013C00UL)
#define EXTI_IMR        (*(volatile uint32_t *)(EXTI_BASE + 0x00UL))
#define EXTI_FTSR       (*(volatile uint32_t *)(EXTI_BASE + 0x0CUL))
#define EXTI_PR         (*(volatile uint32_t *)(EXTI_BASE + 0x14UL))

#define NVIC_ISER0      (*(volatile uint32_t *)(0xE000E100UL))

#define SPI1_BASE       (0x40013000UL)
#define SPI1_CR1        (*(volatile uint32_t *)(SPI1_BASE + 0x00UL))
#define SPI1_SR         (*(volatile uint32_t *)(SPI1_BASE + 0x08UL))
#define SPI1_DR         (*(volatile uint32_t *)(SPI1_BASE + 0x0CUL))

#define USART2_BASE     (0x40004400UL)
#define USART2_SR       (*(volatile uint32_t *)(USART2_BASE + 0x00UL))
#define USART2_DR       (*(volatile uint32_t *)(USART2_BASE + 0x04UL))
#define USART2_BRR      (*(volatile uint32_t *)(USART2_BASE + 0x08UL))
#define USART2_CR1      (*(volatile uint32_t *)(USART2_BASE + 0x0CUL))

#define ADC1_BASE       (0x40012000UL)
#define ADC1_SR         (*(volatile uint32_t *)(ADC1_BASE + 0x00UL))
#define ADC1_CR2        (*(volatile uint32_t *)(ADC1_BASE + 0x08UL))
#define ADC1_SQR3       (*(volatile uint32_t *)(ADC1_BASE + 0x34UL))
#define ADC1_DR         (*(volatile uint32_t *)(ADC1_BASE + 0x4CUL))

#define MCP_RESET       0xC0
#define MCP_WRITE       0x02
#define MCP_READ        0x03
#define MCP_CANSTAT     0x0E
#define MCP_CANCTRL     0x0F
#define MCP_CANINTF     0x2C
#define MCP_CANINTE     0x2B
#define MCP_EFLG        0x2D

#define MCP_CNF1        0x2A
#define MCP_CNF2        0x29
#define MCP_CNF3        0x28

#define MCP_TXB0SIDH    0x31
#define MCP_RTS_TX0     0x81

#define MCP_MODE_NORMAL     0x00
#define MCP_MODE_LOOPBACK   0x40
#define MCP_MODE_CONFIG     0x80

typedef struct __attribute__((packed)) {
    uint8_t counter;
    uint8_t temperature;
    uint8_t potentiometer;
    uint8_t checksum;
} Telemetry_Packet_t;

volatile uint8_t debug_val = 0;
volatile uint8_t raw_spi_test = 0;
volatile uint8_t packet_counter = 0;
volatile uint8_t crc_error_count = 0;
volatile uint8_t can_error_flags = 0;

volatile Telemetry_Packet_t tx_packet = {0};
volatile Telemetry_Packet_t rx_packet = {0};

volatile uint16_t pot_voltage = 0;
volatile uint8_t lm35_temp_c = 0;

void SystemInit(void) {}

void delay_ms(uint32_t ms) {
    volatile uint32_t count = ms * 1600;
    while (count--) __asm__("NOP");
}

void UART2_Init(void) {
    GPIOA_MODER &= ~((3UL << (2 * 2)) | (3UL << (3 * 2)));
    GPIOA_MODER |=  ((2UL << (2 * 2)) | (2UL << (3 * 2)));
    GPIOA_AFRL &= ~((0xFUL << (2 * 4)) | (0xFUL << (3 * 4)));
    GPIOA_AFRL |=  ((7UL << (2 * 4)) | (7UL << (3 * 4)));
    RCC_APB1ENR |= (1UL << 17);
    USART2_BRR = 0x8B;
    USART2_CR1 = (1UL << 3) | (1UL << 2) | (1UL << 13);
}

void UART2_SendChar(char c) {
    while (!(USART2_SR & (1UL << 7)));
    USART2_DR = c;
}

void UART2_SendString(char *str) {
    while (*str) {
        UART2_SendChar(*str++);
    }
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

void MCP2515_Init(void) {
    MCP2515_Reset();
    MCP2515_WriteRegister(MCP_CANCTRL, MCP_MODE_CONFIG);
    delay_ms(10);
    MCP2515_WriteRegister(MCP_CNF1, 0x00);
    MCP2515_WriteRegister(MCP_CNF2, 0x90);
    MCP2515_WriteRegister(MCP_CNF3, 0x02);
    MCP2515_WriteRegister(0x60, 0x40); // Filtre Bypass (Tüm mesajları kabul et)
    MCP2515_WriteRegister(MCP_CANINTE, 0x03);
    MCP2515_WriteRegister(MCP_CANCTRL, MCP_MODE_LOOPBACK);
    delay_ms(10);
}

uint8_t Calculate_CRC(volatile Telemetry_Packet_t *pkt) {
    return (pkt->counter ^ pkt->temperature ^ pkt->potentiometer);
}

void MCP2515_SendPacket(volatile Telemetry_Packet_t *pkt) {
    pkt->checksum = Calculate_CRC(pkt);
    MCP2515_Select();
    SPI1_TransmitReceive(MCP_WRITE);
    SPI1_TransmitReceive(MCP_TXB0SIDH);
    SPI1_TransmitReceive(0x03);
    SPI1_TransmitReceive(0x60);
    SPI1_TransmitReceive(0x00);
    SPI1_TransmitReceive(0x00);
    SPI1_TransmitReceive(0x04);
    SPI1_TransmitReceive(pkt->counter);
    SPI1_TransmitReceive(pkt->temperature);
    SPI1_TransmitReceive(pkt->potentiometer);
    SPI1_TransmitReceive(pkt->checksum);
    MCP2515_Deselect();

    MCP2515_Select();
    SPI1_TransmitReceive(MCP_RTS_TX0);
    MCP2515_Deselect();
}

void MCP2515_ReceivePacket(volatile Telemetry_Packet_t *rx_pkt) {
    MCP2515_Select();
    SPI1_TransmitReceive(0x92);
    rx_pkt->counter       = SPI1_TransmitReceive(0x00);
    rx_pkt->temperature   = SPI1_TransmitReceive(0x00);
    rx_pkt->potentiometer = SPI1_TransmitReceive(0x00);
    rx_pkt->checksum      = SPI1_TransmitReceive(0x00);
    MCP2515_Deselect();

    uint8_t expected_crc = (rx_pkt->counter ^ rx_pkt->temperature ^ rx_pkt->potentiometer);
    if (expected_crc != rx_pkt->checksum) {
        crc_error_count++;
    }
}

void EXTI0_IRQHandler(void) {
    if (EXTI_PR & (1UL << 0)) {
        EXTI_PR |= (1UL << 0);
        can_error_flags = MCP2515_ReadRegister(MCP_EFLG);
        MCP2515_ReceivePacket(&rx_packet);

        char uart_buf[64];
        sprintf(uart_buf, "RX-> C:%u | T:%u | P:%u | CRC:%u | Err:%u\r\n",
                  rx_packet.counter,
                  rx_packet.temperature,
                  rx_packet.potentiometer,
                  rx_packet.checksum,
                  can_error_flags);
        UART2_SendString(uart_buf);
    }
}

int main(void) {
    RCC_AHB1ENR |= (1UL << 0) | (1UL << 1);
    RCC_APB2ENR |= (1UL << 8) | (1UL << 12) | (1UL << 14);

    GPIOA_MODER |= (3UL << (0 * 2)) | (3UL << (1 * 2));
    GPIOA_MODER &= ~((3UL << (4 * 2)) | (3UL << (5 * 2)) | (3UL << (6 * 2)) | (3UL << (7 * 2)));
    GPIOA_MODER |=  ((1UL << (4 * 2)) | (2UL << (5 * 2)) | (2UL << (6 * 2)) | (2UL << (7 * 2)));
    GPIOA_AFRL &= ~((0xFUL << (5 * 4)) | (0xFUL << (6 * 4)) | (0xFUL << (7 * 4)));
    GPIOA_AFRL |=  ((5UL << (5 * 4)) | (5UL << (6 * 4)) | (5UL << (7 * 4)));

    GPIOB_MODER &= ~(3UL << (0 * 2));
    SYSCFG_EXTICR1 &= ~(0xFUL << 0);
    SYSCFG_EXTICR1 |=  (0x1UL << 0);

    EXTI_IMR |= (1UL << 0);
    EXTI_FTSR |= (1UL << 0);
    NVIC_ISER0 |= (1UL << 6);

    SPI1_CR1 = (1UL << 2) | (1UL << 3) | (1UL << 6) | (1UL << 9) | (1UL << 8);
    UART2_Init();

    ADC1_CR2 |= (1UL << 0);
    delay_ms(10);
    MCP2515_Deselect();
    delay_ms(50);

    MCP2515_Init();
    UART2_SendString("STM32F407 CAN Telemetry & Ground Station Started!\r\n");

    while (1) {
        uint16_t adc_lm35 = ADC_Read(0);
        lm35_temp_c = (uint8_t)(adc_lm35 * 330 / 4095);
        pot_voltage = ADC_Read(1);

        tx_packet.counter = packet_counter;
        tx_packet.temperature = lm35_temp_c;
        tx_packet.potentiometer = (uint8_t)(pot_voltage >> 4);

        MCP2515_SendPacket(&tx_packet);
        packet_counter++;
        delay_ms(500);
    }
}
