#include <stdint.h>

#define RCC_BASE        (0x40023800UL)
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x44UL))

#define GPIOA_BASE      (0x40020000UL)
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_AFRH      (*(volatile uint32_t *)(GPIOA_BASE + 0x24UL))

#define ADC1_BASE       (0x40012000UL)
#define ADC_SR          (*(volatile uint32_t *)(ADC1_BASE + 0x00UL))
#define ADC_CR2         (*(volatile uint32_t *)(ADC1_BASE + 0x08UL))
#define ADC_SQR3        (*(volatile uint32_t *)(ADC1_BASE + 0x34UL))
#define ADC_CCR         (*(volatile uint32_t *)(0x40012300UL))

#define USART1_BASE     (0x40011000UL)
#define USART1_SR       (*(volatile uint32_t *)(USART1_BASE + 0x00UL))
#define USART1_DR       (*(volatile uint32_t *)(USART1_BASE + 0x04UL))
#define USART1_BRR      (*(volatile uint32_t *)(USART1_BASE + 0x08UL))
#define USART1_CR1      (*(volatile uint32_t *)(USART1_BASE + 0x0CUL))

void SystemInit(void) {}

void delay_ms(uint32_t ms) {
    volatile uint32_t count = ms * 1600;
    while (count--) __asm__("NOP");
}

void USART1_SendString(char *str) {
    while (*str) {
        while (!(USART1_SR & (1UL << 7))); // TX register boşalmasını bekle
        USART1_DR = *str++;
    }
}

// Basit float değerini string'e çeviren yardımcı fonksiyon (sprintf yükünden kaçınmak için)
void float_to_str(float val, char *buf) {
    int int_part = (int)val;
    int frac_part = (int)((val - int_part) * 100); // virgülden sonra 2 basamak
    if (frac_part < 0) frac_part = -frac_part;

    // Basit manuel formatlama: Örn. "3.29"
    int i = 0;
    // Tam kısım
    if (int_part == 0) {
        buf[i++] = '0';
    } else {
        int temp = int_part;
        int digits = 0;
        while (temp > 0) { digits++; temp /= 10; }
        temp = int_part;
        for (int j = digits - 1; j >= 0; j--) {
            buf[i++] = (temp % 10) + '0';
            temp /= 10;
        }
    }
    buf[i++] = '.';
    // Ondalık kısım
    buf[i++] = (frac_part / 10) + '0';
    buf[i++] = (frac_part % 10) + '0';
    buf[i] = '\0';
}

uint32_t ADC_Read(uint8_t channel) {
    ADC_SQR3 = channel;             // Kanalı seç (0: PA0, 1: PA1)
    ADC_CR2 |= (1UL << 30);         // Yazılımsal çevrimi başlat (SWSTART)
    while (!(ADC_SR & (1UL << 1))); // Çevrimin bitmesini bekle (EOC)
    return (*(volatile uint32_t *)(ADC1_BASE + 0x4CUL)); // Veriyi oku (DR)
}

int main(void) {
    // 1. Clock Aktif Etme
    RCC_AHB1ENR |= (1UL << 0); // GPIOA Clock
    RCC_APB2ENR |= (1UL << 8); // ADC1 Clock
    RCC_APB2ENR |= (1UL << 4); // USART1 Clock

    // 2. PA0 ve PA1 Analog Mod (ADC), PA9 Alternatif Fonksiyon (USART1_TX)
    GPIOA_MODER |= (3UL << (0 * 2)) | (3UL << (1 * 2)); // PA0, PA1 Analog
    GPIOA_MODER &= ~(3UL << (9 * 2));
    GPIOA_MODER |=  (2UL << (9 * 2));                   // PA9 AF mode

    GPIOA_AFRH  &= ~(0xFUL << ((9 - 8) * 4));
    GPIOA_AFRH  |=  (7UL << ((9 - 8) * 4));             // AF7 (USART1)

    // 3. ADC Ayarları
    ADC_CCR = (1UL << 16); // Prescaler PCLK2/4
    ADC_CR2 |= (1UL << 0); // ADC Aktif (ADON)

    // 4. USART1 Ayarları (1200 Baud)
    USART1_BRR  = 0x3412;
    USART1_CR1  = (1UL << 3) | (1UL << 13); // TE ve UE aktif

    char telemetry_msg[64];
    char pot_str[16];
    char temp_str[16];

    while (1) {
        // ADC Örneklemeleri
        uint32_t pot_raw = ADC_Read(1);  // PA1 -> Potansiyometre
        uint32_t lm35_raw = ADC_Read(0); // PA0 -> LM35

        // Voltaj ve Sıcaklık Hesaplamaları
        float pot_voltage = (pot_raw * 3.3f) / 4095.0f;
        float lm35_temp_c = ((lm35_raw * 3.3f) / 4095.0f) * 100.0f;

        float_to_str(pot_voltage, pot_str);
        float_to_str(lm35_temp_c, temp_str);

        // Mesajı oluştur: Örn -> POT:3.29V | TEMP:32.40C\r\n
        // Manuel string birleştirme
        int idx = 0;
        char prefix1[] = "POT:";
        for(int k=0; prefix1[k]!='\0'; k++) telemetry_msg[idx++] = prefix1[k];
        for(int k=0; pot_str[k]!='\0'; k++) telemetry_msg[idx++] = pot_str[k];

        char prefix2[] = "V | TEMP:";
        for(int k=0; prefix2[k]!='\0'; k++) telemetry_msg[idx++] = prefix2[k];
        for(int k=0; temp_str[k]!='\0'; k++) telemetry_msg[idx++] = temp_str[k];

        char suffix[] = "C\r\n";
        for(int k=0; suffix[k]!='\0'; k++) telemetry_msg[idx++] = suffix[k];
        telemetry_msg[idx] = '\0';

        // UART üzerinden gönder
        USART1_SendString(telemetry_msg);

        delay_ms(1000);
    }
}
