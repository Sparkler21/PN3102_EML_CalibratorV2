#include <SPI.h>

static const uint8_t PIN_CS   = D7;
static const uint8_t PIN_DRDY = D6;
static const uint8_t PIN_PDWN = D2;

SPISettings adsSpiSettings(1000000, MSBFIRST, SPI_MODE1);

// ADS1256 commands
static const uint8_t CMD_WAKEUP  = 0x00;
static const uint8_t CMD_RDATA   = 0x01;
static const uint8_t CMD_SDATAC  = 0x0F;
static const uint8_t CMD_WREG    = 0x50;
static const uint8_t CMD_SELFCAL = 0xF0;
static const uint8_t CMD_SYNC    = 0xFC;

// ADS1256 registers
static const uint8_t REG_STATUS = 0x00;
static const uint8_t REG_MUX    = 0x01;
static const uint8_t REG_ADCON  = 0x02;
static const uint8_t REG_DRATE  = 0x03;

// Settings
static const uint8_t DRATE_30SPS = 0x53;
static const uint8_t PGA_64      = 0x06;

int32_t offset = 0;
float filteredCounts = 0.0f;
const float alpha = 0.1f;

// Replace after calibration
float scaleKgPerCount = 0.00001f;

void csLow()  { digitalWrite(PIN_CS, LOW); }
void csHigh() { digitalWrite(PIN_CS, HIGH); }

bool adsWaitDRDY(uint32_t timeoutMs = 100) {
  uint32_t t0 = millis();
  while (digitalRead(PIN_DRDY) == HIGH) {
    if (millis() - t0 > timeoutMs) {
      Serial.println("DRDY timeout");
      return false;
    }
  }
  return true;
}

void adsSendCommand(uint8_t cmd) {
  SPI.beginTransaction(adsSpiSettings);
  csLow();
  SPI.transfer(cmd);
  csHigh();
  SPI.endTransaction();
}

void adsWriteRegister(uint8_t reg, uint8_t value) {
  SPI.beginTransaction(adsSpiSettings);
  csLow();
  SPI.transfer(CMD_WREG | reg);
  SPI.transfer(0x00);
  SPI.transfer(value);
  csHigh();
  SPI.endTransaction();
  delayMicroseconds(10);
}

void adsSetChannel(uint8_t pos, uint8_t neg) {
  uint8_t mux = (pos << 4) | (neg & 0x0F);
  adsWriteRegister(REG_MUX, mux);

  SPI.beginTransaction(adsSpiSettings);
  csLow();
  SPI.transfer(CMD_SYNC);
  delayMicroseconds(4);
  SPI.transfer(CMD_WAKEUP);
  csHigh();
  SPI.endTransaction();

  delayMicroseconds(10);
}

int32_t adsReadData() {
  uint8_t b0, b1, b2;
  int32_t value = 0;

  if (!adsWaitDRDY()) {
    return 0;
  }

  SPI.beginTransaction(adsSpiSettings);
  csLow();
  SPI.transfer(CMD_RDATA);
  delayMicroseconds(10);

  b0 = SPI.transfer(0xFF);
  b1 = SPI.transfer(0xFF);
  b2 = SPI.transfer(0xFF);

  csHigh();
  SPI.endTransaction();

  value = ((int32_t)b0 << 16) | ((int32_t)b1 << 8) | b2;

  if (value & 0x800000) {
    value |= 0xFF000000;
  }

  return value;
}

void adsInit() {
  pinMode(PIN_CS, OUTPUT);
  csHigh();

  pinMode(PIN_DRDY, INPUT_PULLUP);

  pinMode(PIN_PDWN, OUTPUT);
  digitalWrite(PIN_PDWN, HIGH);

  SPI.begin();

  digitalWrite(PIN_PDWN, LOW);
  delay(10);
  digitalWrite(PIN_PDWN, HIGH);
  delay(10);

  adsSendCommand(CMD_SDATAC);
  delay(2);

  adsWaitDRDY();

  adsWriteRegister(REG_STATUS, 0x04);      // ACAL on
  adsWriteRegister(REG_ADCON, PGA_64);     // gain 64
  adsWriteRegister(REG_DRATE, DRATE_30SPS);

  delay(2);
  adsSendCommand(CMD_SELFCAL);
  delay(10);
}

void tare(uint16_t samples = 20) {
  int32_t sum = 0;

  for (uint16_t i = 0; i < samples; i++) {
    sum += adsReadData();
    delay(20);
  }

  offset = sum / samples;
  filteredCounts = 0.0f;

  Serial.print("Offset = ");
  Serial.println(offset);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("Starting ADS1256 single-channel test...");
  adsInit();

  adsSetChannel(0, 1);
  adsReadData();
  delay(50);

  Serial.println("Taring...");
  tare();

  Serial.println("Ready.");
}

void loop() {
  int32_t raw = adsReadData();
  int32_t counts = raw - offset;

  filteredCounts = (1.0f - alpha) * filteredCounts + alpha * (float)counts;
  float weightKg = filteredCounts * scaleKgPerCount;

  Serial.print("Counts: ");
  Serial.print(counts);
  Serial.print("  Filtered: ");
  Serial.print(filteredCounts, 1);
  Serial.print("  Weight kg: ");
  Serial.println(weightKg, 4);

  delay(100);
}