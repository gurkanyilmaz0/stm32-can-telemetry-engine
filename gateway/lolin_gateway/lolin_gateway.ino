#include <SoftwareSerial.h>

// D1 pini RX (STM32'nin PA9 pininden gelen kablo buraya takılacak)
SoftwareSerial stmSerial(D1, D2); // D2 boş kalabilir

void setup() {
  Serial.begin(115200);   // Bilgisayar ekranı (Serial Monitor) hızı
  stmSerial.begin(1200);  // STM32 ile birebir eşleşen 1200 baud hızı
  Serial.println("\n=== YER ISTASYONU DINLEMEDE (D1) ===");
}

void loop() {
  while (stmSerial.available() > 0) {
    char c = stmSerial.read();
    Serial.write(c); // STM32'den gelen veriyi doğrudan bilgisayar ekranına bas
  }
}