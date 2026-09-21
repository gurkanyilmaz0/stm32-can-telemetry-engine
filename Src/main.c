#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

#define GPIOC_BASE      (0x40020800UL)
#define GPIOC_MODER     (*(volatile uint32_t *)(GPIOC_BASE + 0x00UL))
#define GPIOC_AFRH      (*(volatile uint32_t *)(GPIOC_BASE + 0x24UL))

#define SYSCFG_BASE     (0x40013800UL)
#define SYSCFG_EXTICR1  (*(volatile uint32_t *)(SYSCFG_BASE + 0x08UL))

#define EXTI_BASE       (0x40013C00UL)
#define EXTI_IMR        (*(volatile uint32_t *)(EXTI_BASE + 0x00UL))
#define EXTI_FTSR       (*(volatile uint32_t *)(EXTI_BASE + 0x0CUL))
#define EXTI_PR         (*(volatile uint32_t *)(EXTI_BASE + 0x14UL))

#define NVIC_ISER0      (*(volatile uint32_t *)(0xE000E100UL))
#define NVIC_ISER1      (*(volatile uint32_t *)(0xE000E104UL))

#define SPI1_BASE       (0x40013000UL)
#define SPI1_CR1        (*(volatile uint32_t *)(SPI1_BASE + 0x00UL))
#define SPI1_SR         (*(volatile uint32_t *)(SPI1_BASE + 0x08UL))
#define SPI1_DR         (*(volatile uint32_t *)(SPI1_BASE + 0x0CUL))

#define USART2_BASE     (0x40004400UL)
#define USART2_SR       (*(volatile uint32_t *)(USART2_BASE + 0x00UL))
#define USART2_DR       (*(volatile uint32_t *)(USART2_BASE + 0x04UL))
#define USART2_BRR      (*(volatile uint32_t *)(USART2_BASE + 0x08UL))
#define USART2_CR1      (*(volatile uint32_t *)(USART2_BASE + 0x0CUL))

#define USART3_BASE     (0x40004800UL)
#define USART3_SR       (*(volatile uint32_t *)(USART3_BASE + 0x00UL))
#define USART3_DR       (*(volatile uint32_t *)(USART3_BASE + 0x04UL))
#define USART3_BRR      (*(volatile uint32_t *)(USART3_BASE + 0x08UL))
#define USART3_CR1      (*(volatile uint32_t *)(USART3_BASE + 0x0CUL))

#define ADC1_BASE       (0x40012000UL)
#define ADC1_SR         (*(volatile uint32_t *)(ADC1_BASE + 0x00UL))
#define ADC1_CR2        (*(volatile uint32_t *)(ADC1_BASE + 0x08UL))
#define ADC1_SMPR1      (*(volatile uint32_t *)(ADC1_BASE + 0x1CUL))
#define ADC1_SQR3       (*(volatile uint32_t *)(ADC1_BASE + 0x34UL))
#define ADC1_CCR        (*(volatile uint32_t *)(0x40012304UL))
#define ADC1_DR         (*(volatile uint32_t *)(ADC1_BASE + 0x4CUL))

#define MCP_RESET       0xC0
#define MCP_WRITE       0x02
#define MCP_READ        0x03
#define MCP_CANCTRL     0x0F
#define MCP_CANINTE     0x2B
#define MCP_EFLG        0x2D
#define MCP_CNF1        0x2A
#define MCP_CNF2        0x29
#define MCP_CNF3        0x28
#define MCP_TXB0SIDH    0x31
#define MCP_RTS_TX0     0x81
#define MCP_MODE_LOOPBACK   0x40
#define MCP_MODE_CONFIG     0x80

typedef struct __attribute__((packed)) {
    uint8_t counter;
    uint8_t temperature;
    uint8_t mcu_temperature;
    uint8_t potentiometer;
    uint8_t checksum;
} Telemetry_Packet_t;

typedef struct {
    char status;
    char latitude[12];
    char lat_dir;
    char longitude[13];
    char lon_dir;
    uint8_t fixed;
    uint8_t satellites;
    uint8_t fix_quality;
    char utc_time[10];
    char speed_knots[8];
    char course[8];
    char hdop[8];
    char altitude[10];
    char fix_mode[4];
} GPS_Data_t;

volatile uint8_t packet_counter = 0;
volatile uint8_t can_error_flags = 0;
volatile char last_gps_char = 0;
volatile uint32_t gps_char_count = 0;
char gps_rx_buffer[128];
volatile uint8_t gps_buffer_index = 0;
volatile uint8_t gps_line_ready = 0;
char latest_nmea_sentence[128];
volatile GPS_Data_t gps_data = {0};

volatile Telemetry_Packet_t tx_packet = {0};

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

void UART3_Init(void) {
    RCC_AHB1ENR |= (1UL << 2);
    GPIOC_MODER &= ~((3UL << (10 * 2)) | (3UL << (11 * 2)));
    GPIOC_MODER |=  ((2UL << (10 * 2)) | (2UL << (11 * 2)));
    GPIOC_AFRH &= ~((0xFUL << ((10 - 8) * 4)) | (0xFUL << ((11 - 8) * 4)));
    GPIOC_AFRH |=  ((7UL << ((10 - 8) * 4)) | (7UL << ((11 - 8) * 4)));
    RCC_APB1ENR |= (1UL << 18);
    USART3_BRR = 0x683;
    USART3_CR1 = (1UL << 2) | (1UL << 3) | (1UL << 13) | (1UL << 5);
    NVIC_ISER1 |= (1UL << (39 - 32));
}

void UART2_SendChar(char c) {
    while (!(USART2_SR & (1UL << 7)));
    USART2_DR = c;
}

void UART2_SendString(const char *str) {
    while (*str) UART2_SendChar(*str++);
}

uint16_t ADC_Read(uint8_t channel) {
    ADC1_SQR3 = channel;
    ADC1_CR2 |= (1UL << 30);
    uint32_t timeout = 0x5000;
    while (!(ADC1_SR & (1UL << 1))) if (--timeout == 0) return 0;
    return (uint16_t)ADC1_DR;
}

uint8_t SPI1_TransmitReceive(uint8_t byte) {
    uint32_t timeout = 0x5000;
    while (!(SPI1_SR & (1UL << 1))) if (--timeout == 0) return 0xEE;
    *(volatile uint8_t *)&SPI1_DR = byte;
    timeout = 0x5000;
    while (!(SPI1_SR & (1UL << 0))) if (--timeout == 0) return 0xAA;
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
    MCP2515_WriteRegister(0x60, 0x40);
    MCP2515_WriteRegister(MCP_CANINTE, 0x03);
    MCP2515_WriteRegister(MCP_CANCTRL, MCP_MODE_LOOPBACK);
    delay_ms(10);
}

uint8_t Calculate_CRC(volatile Telemetry_Packet_t *pkt) {
    return (pkt->counter ^ pkt->temperature ^ pkt->mcu_temperature ^ pkt->potentiometer);
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
    SPI1_TransmitReceive(pkt->mcu_temperature);
    SPI1_TransmitReceive(pkt->potentiometer);
    MCP2515_Deselect();

    MCP2515_Select();
    SPI1_TransmitReceive(MCP_RTS_TX0);
    MCP2515_Deselect();
}

void EXTI0_IRQHandler(void) {
    if (EXTI_PR & (1UL << 0)) {
        EXTI_PR |= (1UL << 0);
        can_error_flags = MCP2515_ReadRegister(MCP_EFLG);
    }
}

void USART3_IRQHandler(void) {
    if (USART3_SR & (1UL << 5)) {
        char c = (char)USART3_DR;
        last_gps_char = c;
        gps_char_count++;

        if (gps_line_ready == 0) {
            if (c == '\n') {
                gps_rx_buffer[gps_buffer_index] = '\0';
                strcpy(latest_nmea_sentence, gps_rx_buffer);
                gps_buffer_index = 0;
                gps_line_ready = 1;
            } else if (c != '\r' && gps_buffer_index < sizeof(gps_rx_buffer) - 1) {
                gps_rx_buffer[gps_buffer_index++] = c;
            } else if (c != '\r') {
                gps_buffer_index = 0;
            }
        }
    }
}

/* FIX: NMEA checksum validation ("*HH" at the end of the sentence).
 * Without this, corrupted/truncated lines were silently parsed and could
 * populate gps_data with garbage. */
uint8_t NMEA_ChecksumValid(const char *sentence) {
    if (sentence[0] != '$') return 0;
    const char *star = strchr(sentence, '*');
    if (star == NULL || strlen(star) < 3) return 0;

    uint8_t calc = 0;
    for (const char *p = sentence + 1; p < star; p++) calc ^= (uint8_t)*p;

    uint8_t given = (uint8_t)strtol(star + 1, NULL, 16);
    return calc == given;
}

float NMEA_To_Decimal(char *nmea_str, char dir) {
    if (strlen(nmea_str) == 0) return 0.0f;
    float raw = atof(nmea_str);
    int degrees = (int)(raw / 100);
    float minutes = raw - (degrees * 100.0f);
    float decimal = degrees + (minutes / 60.0f);
    if (dir == 'S' || dir == 'W') decimal = -decimal;
    return decimal;
}

void Parse_NMEA_GPRMC(char *sentence) {
    char temp[128];
    strncpy(temp, sentence, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    char *field_start = temp;
    uint8_t field_index = 0;

    while (field_start != NULL) {
        char *comma = strchr(field_start, ',');
        if (comma != NULL) *comma = '\0';
        size_t len = strlen(field_start);

        if (field_index == 1 && len > 0) { strncpy(gps_data.utc_time, field_start, sizeof(gps_data.utc_time)-1); }
        else if (field_index == 2 && len > 0) gps_data.status = field_start[0];
        else if (field_index == 3 && len > 0) { strncpy(gps_data.latitude, field_start, sizeof(gps_data.latitude) - 1); }
        else if (field_index == 4 && len > 0) gps_data.lat_dir = field_start[0];
        else if (field_index == 5 && len > 0) { strncpy(gps_data.longitude, field_start, sizeof(gps_data.longitude) - 1); }
        else if (field_index == 6 && len > 0) gps_data.lon_dir = field_start[0];
        else if (field_index == 7 && len > 0) { strncpy(gps_data.speed_knots, field_start, sizeof(gps_data.speed_knots)-1); }
        else if (field_index == 8 && len > 0) { strncpy(gps_data.course, field_start, sizeof(gps_data.course)-1); }

        field_start = (comma != NULL) ? (comma + 1) : NULL;
        field_index++;
    }
}

void Parse_NMEA_GPGGA(char *sentence) {
    char temp[128];
    strncpy(temp, sentence, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    char *field_start = temp;
    uint8_t field_index = 0;

    while (field_start != NULL) {
        char *comma = strchr(field_start, ',');
        if (comma != NULL) *comma = '\0';
        size_t len = strlen(field_start);

        if (field_index == 6 && len > 0) gps_data.fix_quality = (uint8_t)(field_start[0] - '0');
        else if (field_index == 7 && len > 0) gps_data.satellites = (uint8_t)atoi(field_start);
        else if (field_index == 8 && len > 0) { strncpy(gps_data.hdop, field_start, sizeof(gps_data.hdop)-1); }
        else if (field_index == 9 && len > 0) { strncpy(gps_data.altitude, field_start, sizeof(gps_data.altitude)-1); }

        field_start = (comma != NULL) ? (comma + 1) : NULL;
        field_index++;
    }
    gps_data.fixed = (gps_data.fix_quality > 0) ? 1 : 0;
}

void Parse_NMEA_GPGSA(char *sentence) {
    char temp[128];
    strncpy(temp, sentence, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    char *field_start = temp;
    uint8_t field_index = 0;

    while (field_start != NULL) {
        char *comma = strchr(field_start, ',');
        if (comma != NULL) *comma = '\0';
        size_t len = strlen(field_start);

        if (field_index == 2 && len > 0) {
            if (field_start[0] == '2') strcpy(gps_data.fix_mode, "2D");
            else if (field_start[0] == '3') strcpy(gps_data.fix_mode, "3D");
            else strcpy(gps_data.fix_mode, "NO");
        }

        field_start = (comma != NULL) ? (comma + 1) : NULL;
        field_index++;
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

    /* FIX: SPI1 prescaler was fPCLK2/4 (~21 MHz on an 84 MHz APB2), well above
     * the MCP2515's 10 MHz SPI limit and a likely cause of corrupted SPI
     * transactions / CAN link errors. BR[2:0] = 011 -> /16 (~5.25 MHz),
     * comfortably inside spec. */
    SPI1_CR1 = (1UL << 2) | (3UL << 3) | (1UL << 6) | (1UL << 9) | (1UL << 8);
    UART2_Init();
    UART3_Init();

    delay_ms(1000);
    ADC1_CR2 |= (1UL << 0);
    ADC1_CCR |= (1UL << 23); // Dahili sicaklik sensoru ve VREFINT aktif
    /* FIX: SMPR1 sample-time field for channel 18 (internal temp sensor) is
     * bits [26:24], not [20:18]. The old shift value was configuring the
     * sample time for channel 16 instead, leaving channel 18 at its
     * (too-short) reset sample time and effectively fixing MCU temp at 0. */
    ADC1_SMPR1 |= (7UL << 24); // Kanal 18 icin ornekleme suresi
    delay_ms(10);
    MCP2515_Deselect();
    delay_ms(50);
    MCP2515_Init();

    uint32_t telemetry_timer = 0;

    while (1) {
        if (gps_line_ready) {
            char sentence_copy[128];
            __asm__ volatile("cpsid i");
            gps_line_ready = 0;
            strncpy(sentence_copy, latest_nmea_sentence, sizeof(sentence_copy) - 1);
            sentence_copy[sizeof(sentence_copy) - 1] = '\0';
            __asm__ volatile("cpsie i");

            /* FIX: validate checksum before trusting the sentence. Sentences
             * without a '*HH' checksum (or a corrupted one) are ignored
             * instead of being parsed into gps_data. */
            if (strlen(sentence_copy) >= 6 && sentence_copy[0] == '$' &&
                NMEA_ChecksumValid(sentence_copy)) {
                if (memcmp(sentence_copy + 3, "RMC", 3) == 0) Parse_NMEA_GPRMC(sentence_copy);
                else if (memcmp(sentence_copy + 3, "GGA", 3) == 0) Parse_NMEA_GPGGA(sentence_copy);
                else if (memcmp(sentence_copy + 3, "GSA", 3) == 0) Parse_NMEA_GPGSA(sentence_copy);
            }
        }

        delay_ms(2);
        telemetry_timer++;

        if (telemetry_timer >= 250) {
            telemetry_timer = 0;

            uint16_t adc_lm35 = ADC_Read(0);
            uint8_t lm35_temp = (uint8_t)(adc_lm35 * 330 / 4095);

            // Dahili Sicaklik Sensoru (Channel 18) Okuma Formulu (STM32F4)
            uint16_t adc_mcu_raw = ADC_Read(18);
            float sense_voltage = (float)adc_mcu_raw * 3.3f / 4095.0f;
            uint8_t mcu_temp = (uint8_t)(((sense_voltage - 0.76f) / 0.0025f) + 25.0f);

            uint16_t pot_val = ADC_Read(1);

            tx_packet.counter = packet_counter;
            tx_packet.temperature = lm35_temp;
            tx_packet.mcu_temperature = mcu_temp;
            tx_packet.potentiometer = (uint8_t)(pot_val >> 4);
            MCP2515_SendPacket(&tx_packet);
            packet_counter++;

            char lat_str[15] = "0.000000";
            char lon_str[15] = "0.000000";

            if (gps_data.status == 'A') {
                float lat_f = NMEA_To_Decimal((char*)gps_data.latitude, gps_data.lat_dir);
                float lon_f = NMEA_To_Decimal((char*)gps_data.longitude, gps_data.lon_dir);

                int lat_int = (int)lat_f;
                int lat_frac = abs((int)((lat_f - lat_int) * 1000000));
                sprintf(lat_str, "%d.%06d", lat_int, lat_frac);

                int lon_int = (int)lon_f;
                int lon_frac = abs((int)((lon_f - lon_int) * 1000000));
                sprintf(lon_str, "%d.%06d", lon_int, lon_frac);
            }

            char bridge_msg[220];
            sprintf(bridge_msg, "RX-> C:%d | T:%d | MCU_T:%d | P:%d | Lat:%s | Lon:%s | Spd:%s | Cog:%s | Time:%s | Hdop:%s | Alt:%s | Mode:%s | Sat:%d\n",
                    tx_packet.counter, tx_packet.temperature, tx_packet.mcu_temperature, tx_packet.potentiometer,
                    lat_str, lon_str,
                    strlen(gps_data.speed_knots) > 0 ? gps_data.speed_knots : "0.0",
                    strlen(gps_data.course) > 0 ? gps_data.course : "0.0",
                    strlen(gps_data.utc_time) > 0 ? gps_data.utc_time : "000000",
                    strlen(gps_data.hdop) > 0 ? gps_data.hdop : "9.9",
                    strlen(gps_data.altitude) > 0 ? gps_data.altitude : "0.0",
                    strlen(gps_data.fix_mode) > 0 ? gps_data.fix_mode : "NO",
                    gps_data.satellites);

            UART2_SendString(bridge_msg);
        }
    }
}
