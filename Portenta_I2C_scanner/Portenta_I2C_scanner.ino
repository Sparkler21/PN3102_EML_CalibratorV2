#include <Wire.h>

void setup() {
  Serial.begin(115200);
  delay(1500);
  Wire.begin();
  Serial.println("Scanning...");
}

void loop() {
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();

    if (err == 0) {
      Serial.print("Found device at 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
    }
  }

  Serial.println("Scan done");
  delay(3000);
}