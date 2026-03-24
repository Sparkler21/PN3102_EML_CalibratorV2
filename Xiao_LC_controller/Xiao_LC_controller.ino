//Mark Dutton 
// 24th March 2026
#include <SPI.h>
#include <Wire.h>
#include <FlashStorage_SAMD.h>

void onI2CReceive(int numBytes);
void onI2CRequest();

static const uint8_t PIN_CS      = D7;
static const uint8_t PIN_DRDY    = D6;
static const uint8_t PIN_PDWN    = D2;
static const uint8_t PIN_MODESEL = D1;   // jumper to GND = calibration mode

static const uint8_t I2C_ADDR = 0x2A;

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

enum RunMode {
  MODE_OPERATION,
  MODE_CALIBRATION
};

RunMode runMode = MODE_OPERATION;

struct CalibrationData {
  uint32_t magic;
  uint32_t version;
  int32_t offsets[4];
  float scales[4];
};

FlashStorage(calStore, CalibrationData);

static const uint32_t CAL_MAGIC   = 0x4C43414C;   // "LCAL"
static const uint32_t CAL_VERSION = 1;

struct __attribute__((packed)) WeightPacket {
  float lc1;
  float lc2;
  float lc3;
  float lc4;
  uint32_t sequence;
};

static_assert(sizeof(WeightPacket) == 20, "Packet size incorrect");

struct LoadCellChannel {
  uint8_t pos;
  uint8_t neg;
  int32_t offset;
  float filteredCounts;
  float scaleKgPerCount;
  int32_t counts;
  float weightKg;
};

LoadCellChannel lc[4] = {
  {0, 1, 0, 0.0f, 0.00002471f, 0, 0.0f},
  {2, 3, 0, 0.0f, 0.00002471f, 0, 0.0f},
  {4, 5, 0, 0.0f, 0.00002471f, 0, 0.0f},
  {6, 7, 0, 0.0f, 0.00002471f, 0, 0.0f}
};

const float alpha = 0.10f;
volatile uint32_t packetSequence = 0;

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

  adsWriteRegister(REG_STATUS, 0x04);
  adsWriteRegister(REG_ADCON, PGA_64);
  adsWriteRegister(REG_DRATE, DRATE_30SPS);

  delay(2);
  adsSendCommand(CMD_SELFCAL);
  delay(10);
}

int32_t readChannelRaw(uint8_t pos, uint8_t neg) {
  adsSetChannel(pos, neg);
  return adsReadData();
}

void tareOneChannel(uint8_t index, uint16_t samples = 30) {
  int64_t sum = 0;
  for (uint16_t i = 0; i < samples; i++) {
    sum += readChannelRaw(lc[index].pos, lc[index].neg);
    delay(20);
  }
  lc[index].offset = (int32_t)(sum / samples);
  lc[index].filteredCounts = 0.0f;
  lc[index].counts = 0;
  lc[index].weightKg = 0.0f;
}

void tareAllChannels(uint16_t samples = 30) {
  Serial.println("Taring all channels...");
  for (uint8_t i = 0; i < 4; i++) {
    tareOneChannel(i, samples);
    Serial.print("LC");
    Serial.print(i + 1);
    Serial.print(" offset = ");
    Serial.println(lc[i].offset);
  }

  saveCalibrationToFlash();
  Serial.println("Offsets saved to flash.");
}

void updateAllChannels() {
  for (uint8_t i = 0; i < 4; i++) {
    int32_t raw = readChannelRaw(lc[i].pos, lc[i].neg);
    lc[i].counts = raw - lc[i].offset;
    lc[i].filteredCounts =
      (1.0f - alpha) * lc[i].filteredCounts +
      alpha * (float)lc[i].counts;
    lc[i].weightKg = lc[i].filteredCounts * lc[i].scaleKgPerCount;
  }

  packetSequence++;
}

void onI2CRequest() {
  WeightPacket pkt;
  pkt.lc1 = lc[0].weightKg;
  pkt.lc2 = lc[1].weightKg;
  pkt.lc3 = lc[2].weightKg;
  pkt.lc4 = lc[3].weightKg;
  pkt.sequence = packetSequence;

  Wire.write((uint8_t *)&pkt, sizeof(pkt));
}

void onI2CReceive(int numBytes) {
  while (Wire.available()) {
    Wire.read(); // discard incoming data
  }
}

void printWeights() {
  Serial.print("LC1: "); Serial.print(lc[0].weightKg, 3);
  Serial.print("  LC2: "); Serial.print(lc[1].weightKg, 3);
  Serial.print("  LC3: "); Serial.print(lc[2].weightKg, 3);
  Serial.print("  LC4: "); Serial.println(lc[3].weightKg, 3);
}

void printCounts() {
  for (uint8_t i = 0; i < 4; i++) {
    Serial.print("LC");
    Serial.print(i + 1);
    Serial.print(" counts=");
    Serial.print(lc[i].counts);
    Serial.print(" filtered=");
    Serial.print(lc[i].filteredCounts, 1);
    Serial.print("   ");
  }
  Serial.println();
}

void printScaleFactors() {
  Serial.println("Scale factors (kg/count):");
  for (uint8_t i = 0; i < 4; i++) {
    Serial.print("LC");
    Serial.print(i + 1);
    Serial.print(" = ");
    Serial.println(lc[i].scaleKgPerCount, 8);
  }
}

void calibrateChannel(uint8_t index, float knownKg) {
  Serial.print("Calibrating LC");
  Serial.print(index + 1);
  Serial.print(" with known mass ");
  Serial.print(knownKg, 3);
  Serial.println(" kg");

  delay(1000);

  for (uint8_t i = 0; i < 20; i++) {
    int32_t raw = readChannelRaw(lc[index].pos, lc[index].neg);
    lc[index].counts = raw - lc[index].offset;
    lc[index].filteredCounts =
      (1.0f - alpha) * lc[index].filteredCounts +
      alpha * (float)lc[index].counts;
    delay(50);
  }

  float counts = lc[index].filteredCounts;

  if (counts == 0.0f) {
    Serial.println("Calibration failed: filtered counts = 0");
    return;
  }

  lc[index].scaleKgPerCount = knownKg / counts;

  Serial.print("LC");
  Serial.print(index + 1);
  Serial.print(" filtered counts = ");
  Serial.println(counts, 1);

  Serial.print("New scale factor = ");
  Serial.println(lc[index].scaleKgPerCount, 8);

  saveCalibrationToFlash();
  Serial.println("Calibration saved to flash.");
}

float readFloatFromSerial() {
  String input = "";

  while (true) {
    while (Serial.available()) {
      char c = Serial.read();

      if (c == '\n' || c == '\r') {
        if (input.length() > 0) {
          return input.toFloat();
        }
      } else {
        input += c;
      }
    }
  }
}

void printCalibrationMenu() {
  Serial.println();
  Serial.println("=== CALIBRATION MODE ===");
  Serial.println("t        - tare all channels");
  Serial.println("w        - show live weights");
  Serial.println("c        - show live counts");
  Serial.println("p        - print scale factors");
  Serial.println("1,kg     - calibrate LC1  e.g. 1,1.906");
  Serial.println("2,kg     - calibrate LC2  e.g. 2,1.906");
  Serial.println("3,kg     - calibrate LC3  e.g. 3,1.906");
  Serial.println("4,kg     - calibrate LC4  e.g. 4,1.906");
  Serial.println("s        - save calibration to flash");
  Serial.println("l        - load calibration from flash");
  Serial.println("d        - print stored calibration");
  Serial.println("h        - show this menu");
  Serial.println();
}

void handleCalibrationSerial() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();

  if (line.length() == 0) return;

  if (line.equalsIgnoreCase("t")) {
    tareAllChannels(30);
    return;
  }

  if (line.equalsIgnoreCase("w")) {
    Serial.println("Live weights for 10 seconds...");
    for (uint16_t i = 0; i < 100; i++) {
      updateAllChannels();
      printWeights();
      delay(100);
    }
    return;
  }

  if (line.equalsIgnoreCase("c")) {
    Serial.println("Live counts for 10 seconds...");
    for (uint16_t i = 0; i < 100; i++) {
      updateAllChannels();
      printCounts();
      delay(100);
    }
    return;
  }

  if (line.equalsIgnoreCase("p")) {
    printScaleFactors();
    return;
  }

  if (line.equalsIgnoreCase("s")) {
    saveCalibrationToFlash();
    Serial.println("Calibration saved to flash.");
    return;
  }

  if (line.equalsIgnoreCase("l")) {
    if (loadCalibrationFromFlash()) {
      Serial.println("Calibration loaded from flash.");
      printCalibrationData();
    } else {
      Serial.println("No valid calibration found in flash.");
    }
    return;
  }

  if (line.equalsIgnoreCase("d")) {
    printCalibrationData();
    return;
  }

  if (line.equalsIgnoreCase("h")) {
    printCalibrationMenu();
    return;
  }

  // Calibration command format: channel,mass
  int commaPos = line.indexOf(',');
  if (commaPos > 0) {
    String chStr = line.substring(0, commaPos);
    String massStr = line.substring(commaPos + 1);

    chStr.trim();
    massStr.trim();

    int channel = chStr.toInt();      // 1..4
    float knownKg = massStr.toFloat();

    if (channel >= 1 && channel <= 4 && knownKg > 0.0f) {
      Serial.print("Calibrating LC");
      Serial.print(channel);
      Serial.print(" with known mass ");
      Serial.print(knownKg, 3);
      Serial.println(" kg");

      calibrateChannel(channel - 1, knownKg);
      return;
    }
  }

  Serial.println("Unknown command. Type h for help.");
}


bool loadCalibrationFromFlash() {
  CalibrationData data;
  calStore.read(data);

  if (data.magic != CAL_MAGIC || data.version != CAL_VERSION) {
    return false;
  }

  for (uint8_t i = 0; i < 4; i++) {
    lc[i].offset = data.offsets[i];
    lc[i].scaleKgPerCount = data.scales[i];
  }

  return true;
}

void saveCalibrationToFlash() {
  CalibrationData data;

  data.magic = CAL_MAGIC;
  data.version = CAL_VERSION;

  for (uint8_t i = 0; i < 4; i++) {
    data.offsets[i] = lc[i].offset;
    data.scales[i] = lc[i].scaleKgPerCount;
  }

  calStore.write(data);
}

void printCalibrationData() {
  Serial.println("Stored calibration:");
  for (uint8_t i = 0; i < 4; i++) {
    Serial.print("LC");
    Serial.print(i + 1);
    Serial.print(" offset=");
    Serial.print(lc[i].offset);
    Serial.print(" scale=");
    Serial.println(lc[i].scaleKgPerCount, 8);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Wire.begin(I2C_ADDR);
  Wire.onRequest(onI2CRequest);
  Wire.onReceive(onI2CReceive);

  pinMode(PIN_MODESEL, INPUT_PULLUP);

  bool calLoaded = loadCalibrationFromFlash();

  if (calLoaded) {
    Serial.println("Calibration loaded from flash.");
  } else {
    Serial.println("No valid calibration in flash. Using defaults.");
    // keep your hard-coded defaults here
    lc[0].scaleKgPerCount = 0.00002471f;
    lc[1].scaleKgPerCount = 0.00002471f;
    lc[2].scaleKgPerCount = 0.00002471f;
    lc[3].scaleKgPerCount = 0.00002471f;
  }

  printCalibrationData();

  adsInit();

  for (uint8_t i = 0; i < 4; i++) {
    int32_t raw = readChannelRaw(lc[i].pos, lc[i].neg);
    lc[i].counts = raw - lc[i].offset;
    lc[i].filteredCounts = (float)lc[i].counts;
    lc[i].weightKg = lc[i].filteredCounts * lc[i].scaleKgPerCount;
  }

  if (digitalRead(PIN_MODESEL) == LOW) {
    runMode = MODE_CALIBRATION;
    Serial.println("Boot mode: CALIBRATION");
    tareAllChannels(30);   // only tare automatically in calibration mode
    printCalibrationMenu();
  } 
  else {
    runMode = MODE_OPERATION;
    Serial.println("Boot mode: OPERATION");

    // Initialise filter from current readings so startup is smooth
    for (uint8_t i = 0; i < 4; i++) {
      int32_t raw = readChannelRaw(lc[i].pos, lc[i].neg);
      lc[i].counts = raw - lc[i].offset;
      lc[i].filteredCounts = (float)lc[i].counts;
      lc[i].weightKg = lc[i].filteredCounts * lc[i].scaleKgPerCount;
    }
  }
}

void loop() {
  if (runMode == MODE_CALIBRATION) {
    handleCalibrationSerial();
  } else {
    updateAllChannels();
    printWeights();
    delay(100);
  }
}