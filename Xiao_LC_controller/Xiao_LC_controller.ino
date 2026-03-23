#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

// =========================
// XIAO pin definitions
// =========================
static const uint8_t PIN_ADS_CS   = D7;
static const uint8_t PIN_ADS_DRDY = D6;
static const uint8_t PIN_ADS_RST  = D2;

// XIAO hardware SPI:
// MOSI = D10
// MISO = D9
// SCK  = D8

// I2C slave address seen by Portenta
static const uint8_t I2C_ADDR = 0x2A;

// =========================
// ADS1256 commands
// =========================
static const uint8_t CMD_WAKEUP  = 0x00;
static const uint8_t CMD_RDATA   = 0x01;
static const uint8_t CMD_RDATAC  = 0x03;
static const uint8_t CMD_SDATAC  = 0x0F;
static const uint8_t CMD_RREG    = 0x10;
static const uint8_t CMD_WREG    = 0x50;
static const uint8_t CMD_SELFCAL = 0xF0;
static const uint8_t CMD_SYNC    = 0xFC;
static const uint8_t CMD_STANDBY = 0xFD;
static const uint8_t CMD_RESET   = 0xFE;

// ADS1256 registers
static const uint8_t REG_STATUS = 0x00;
static const uint8_t REG_MUX    = 0x01;
static const uint8_t REG_ADCON  = 0x02;
static const uint8_t REG_DRATE  = 0x03;

// Data rate settings
static const uint8_t DRATE_100SPS = 0x82;
static const uint8_t DRATE_30SPS  = 0x53;
static const uint8_t DRATE_10SPS  = 0x23;

// PGA settings
static const uint8_t PGA_1   = 0x00;
static const uint8_t PGA_2   = 0x01;
static const uint8_t PGA_4   = 0x02;
static const uint8_t PGA_8   = 0x03;
static const uint8_t PGA_16  = 0x04;
static const uint8_t PGA_32  = 0x05;
static const uint8_t PGA_64  = 0x06;

// SPI mode for ADS1256
SPISettings adsSpiSettings(1000000, MSBFIRST, SPI_MODE1);

// =========================
// Channel data
// =========================
struct LoadCellChannel {
  uint8_t ainP;
  uint8_t ainN;
  int32_t raw;
  float filtered;
  int32_t offset;
  float scaleKgPerCount;   // set by calibration
  float weightKg;
};

LoadCellChannel lc[4] = {
  {0, 1, 0, 0.0f, 0, 1.0f, 0.0f},
  {2, 3, 0, 0.0f, 0, 1.0f, 0.0f},
  {4, 5, 0, 0.0f, 0, 1.0f, 0.0f},
  {6, 7, 0, 0.0f, 0, 1.0f, 0.0f}
};

static const float FILTER_ALPHA = 0.10f;

// =========================
// I2C packet
// 4 floats = 16 bytes
// 1 uint32_t sequence = 4 bytes
// total = 20 bytes
// =========================
struct __attribute__((packed)) WeightPacket {
  float weightKg[4];
  uint32_t sequence;
};

volatile bool tareRequested = false;
volatile uint32_t packetSequence = 0;

// =========================
// Low-level ADS helpers
// =========================
void adsCsLow()  { digitalWrite(PIN_ADS_CS, LOW); }
void adsCsHigh() { digitalWrite(PIN_ADS_CS, HIGH); }

void adsWaitDRDY(uint32_t timeoutMs = 100) {
  uint32_t t0 = millis();
  while (digitalRead(PIN_ADS_DRDY) == HIGH) {
    if ((millis() - t0) > timeoutMs) {
      break;
    }
  }
}

void adsSendCommand(uint8_t cmd) {
  SPI.beginTransaction(adsSpiSettings);
  adsCsLow();
  SPI.transfer(cmd);
  adsCsHigh();
  SPI.endTransaction();
}

void adsWriteRegister(uint8_t reg, uint8_t value) {
  SPI.beginTransaction(adsSpiSettings);
  adsCsLow();
  SPI.transfer(CMD_WREG | reg);
  SPI.transfer(0x00);   // write one register
  SPI.transfer(value);
  adsCsHigh();
  SPI.endTransaction();
  delayMicroseconds(10);
}

uint8_t adsReadRegister(uint8_t reg) {
  uint8_t value = 0;
  SPI.beginTransaction(adsSpiSettings);
  adsCsLow();
  SPI.transfer(CMD_RREG | reg);
  SPI.transfer(0x00);   // read one register
  delayMicroseconds(10);
  value = SPI.transfer(0xFF);
  adsCsHigh();
  SPI.endTransaction();
  return value;
}

void adsSetDifferentialChannel(uint8_t pos, uint8_t neg) {
  uint8_t mux = (pos << 4) | (neg & 0x0F);

  adsWriteRegister(REG_MUX, mux);

  SPI.beginTransaction(adsSpiSettings);
  adsCsLow();
  SPI.transfer(CMD_SYNC);
  delayMicroseconds(4);
  SPI.transfer(CMD_WAKEUP);
  adsCsHigh();
  SPI.endTransaction();

  delayMicroseconds(10);
}

int32_t adsReadData() {
  uint8_t b0, b1, b2;
  int32_t value;

  adsWaitDRDY();

  SPI.beginTransaction(adsSpiSettings);
  adsCsLow();
  SPI.transfer(CMD_RDATA);
  delayMicroseconds(10);

  b0 = SPI.transfer(0xFF);
  b1 = SPI.transfer(0xFF);
  b2 = SPI.transfer(0xFF);

  adsCsHigh();
  SPI.endTransaction();

  value = ((int32_t)b0 << 16) | ((int32_t)b1 << 8) | b2;

  // Sign extend 24-bit to 32-bit
  if (value & 0x800000) {
    value |= 0xFF000000;
  }

  return value;
}

bool ads1256Begin() {
  pinMode(PIN_ADS_CS, OUTPUT);
  adsCsHigh();

  pinMode(PIN_ADS_DRDY, INPUT_PULLUP);

  pinMode(PIN_ADS_RST, OUTPUT);
  digitalWrite(PIN_ADS_RST, HIGH);

  SPI.begin();

  // PDWN reset pulse
  digitalWrite(PIN_ADS_RST, LOW);
  delay(10);
  digitalWrite(PIN_ADS_RST, HIGH);
  delay(10);

  // Stop continuous mode
  adsSendCommand(CMD_SDATAC);
  delay(2);

  adsWaitDRDY();

  // STATUS:
  // bit2 ACAL=1
  // input buffer off
  adsWriteRegister(REG_STATUS, 0x04);

  // ADCON:
  // clock out off, sensor detect off, PGA gain
  adsWriteRegister(REG_ADCON, PGA_64);

  // Lower speed = lower noise
  adsWriteRegister(REG_DRATE, DRATE_30SPS);

  delay(2);
  adsSendCommand(CMD_SELFCAL);
  delay(5);

  return true;
}

// =========================
// Load cell functions
// =========================
void readAllLoadCells() {
  for (int i = 0; i < 4; i++) {
    adsSetDifferentialChannel(lc[i].ainP, lc[i].ainN);
    lc[i].raw = adsReadData();

    if (lc[i].filtered == 0.0f) {
      lc[i].filtered = (float)lc[i].raw;
    } else {
      lc[i].filtered =
        (1.0f - FILTER_ALPHA) * lc[i].filtered +
        FILTER_ALPHA * (float)lc[i].raw;
    }

    lc[i].weightKg = (lc[i].filtered - (float)lc[i].offset) * lc[i].scaleKgPerCount;
  }

  packetSequence++;
}

void tareAllLoadCells(uint16_t samples = 20) {
  float sum[4] = {0, 0, 0, 0};

  for (uint16_t s = 0; s < samples; s++) {
    for (int i = 0; i < 4; i++) {
      adsSetDifferentialChannel(lc[i].ainP, lc[i].ainN);
      int32_t v = adsReadData();
      sum[i] += (float)v;
    }
  }

  for (int i = 0; i < 4; i++) {
    lc[i].offset = (int32_t)(sum[i] / samples);
    lc[i].filtered = (float)lc[i].offset;
    lc[i].weightKg = 0.0f;
  }
}

void printDebug() {
  Serial.print("LC1: "); Serial.print(lc[0].weightKg, 4); Serial.print(" kg, ");
  Serial.print("LC2: "); Serial.print(lc[1].weightKg, 4); Serial.print(" kg, ");
  Serial.print("LC3: "); Serial.print(lc[2].weightKg, 4); Serial.print(" kg, ");
  Serial.print("LC4: "); Serial.print(lc[3].weightKg, 4); Serial.println(" kg");
}

// =========================
// I2C callbacks
// Master reads 20-byte packet
// Master may write 1-byte commands:
// 'T' = tare
// =========================
void onI2CRequest() {
  WeightPacket pkt;
  for (int i = 0; i < 4; i++) {
    pkt.weightKg[i] = lc[i].weightKg;
  }
  pkt.sequence = packetSequence;

  Wire.write((uint8_t *)&pkt, sizeof(pkt));
}

void onI2CReceive(int count) {
  while (Wire.available()) {
    char c = (char)Wire.read();
    if (c == 'T') {
      tareRequested = true;
    }
  }
}

// =========================
// Setup / loop
// =========================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("XIAO ADS1256 coprocessor starting...");

  ads1256Begin();

  // Placeholder scale factors.
  // Replace after calibration for each load cell.
  lc[0].scaleKgPerCount = 0.000001f;
  lc[1].scaleKgPerCount = 0.000001f;
  lc[2].scaleKgPerCount = 0.000001f;
  lc[3].scaleKgPerCount = 0.000001f;

  Serial.println("Taring...");
  tareAllLoadCells(30);

  Wire.begin(I2C_ADDR);
  Wire.onRequest(onI2CRequest);
  Wire.onReceive(onI2CReceive);

  Serial.println("Ready.");
}

void loop() {
  if (tareRequested) {
    noInterrupts();
    tareRequested = false;
    interrupts();

    Serial.println("Tare requested via I2C");
    tareAllLoadCells(30);
  }

  readAllLoadCells();

  static uint32_t lastPrint = 0;
  if (millis() - lastPrint > 500) {
    lastPrint = millis();
    printDebug();
  }

  delay(10);
}