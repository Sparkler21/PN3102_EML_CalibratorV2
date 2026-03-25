#include <Wire.h>

static const uint8_t XIAO_ADDR = 0x2A;

struct __attribute__((packed)) WeightPacket {
  float lc1;
  float lc2;
  float lc3;
  float lc4;
  uint32_t sequence;
};

static_assert(sizeof(WeightPacket) == 20, "Packet size incorrect");

void setup() {
  Serial.begin(115200);
  delay(1500);
  Wire.begin();
  Serial.println("Portenta I2C packet reader starting...");
}

void loop() {
  WeightPacket pkt = {0};

  Wire.requestFrom(XIAO_ADDR, (uint8_t)sizeof(pkt));

  uint8_t *p = (uint8_t *)&pkt;
  uint8_t i = 0;

  while (Wire.available() && i < sizeof(pkt)) {
    p[i++] = Wire.read();
  }

  if (i == sizeof(pkt)) {
    Serial.print("Seq: ");
    Serial.print(pkt.sequence);
    Serial.print("  LC1: ");
    Serial.print(pkt.lc1, 3);
    Serial.print("  LC2: ");
    Serial.print(pkt.lc2, 3);
    Serial.print("  LC3: ");
    Serial.print(pkt.lc3, 3);
    Serial.print("  LC4: ");
    Serial.println(pkt.lc4, 3);
  } else {
    Serial.print("I2C read error, bytes=");
    Serial.println(i);
  }

  delay(500);
}