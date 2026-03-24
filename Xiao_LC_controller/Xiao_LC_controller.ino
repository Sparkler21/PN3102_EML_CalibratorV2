#include <SPI.h>

static const uint8_t PIN_CS      = D7;
static const uint8_t PIN_DRDY    = D6;
static const uint8_t PIN_PDWN    = D2;
static const uint8_t PIN_MODESEL = D1;   // jumper to GND = calibration mode

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
  int32_t sum = 0;
  for (uint16_t i = 0; i < samples; i++) {
    sum += readChannelRaw(lc[index].pos, lc[index].neg);
    delay(20);
  }
  lc[index].offset = sum / samples;
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
    Serial.print(" weight=");
    Serial.print(lc[i].weightKg, 3);
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

  // settle a bit
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
}

float readFloatFromSerial() {
  String input = "";

  // Clear any leftover line endings already in the buffer
  while (Serial.available()) {
    Serial.read();
  }

  // Wait for a full line from the user
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();

      if (c == '\n' || c == '\r') {
        if (input.length() > 0) {
          break;
        }
      } else {
        input += c;
      }
    }
  }

  return input.toFloat();
}

void printCalibrationMenu() {
  Serial.println();
  Serial.println("=== CALIBRATION MODE ===");
  Serial.println("t  - tare all channels");
  Serial.println("c  - show live counts");
  Serial.println("w  - show live weights");
  Serial.println("1  - calibrate LC1");
  Serial.println("2  - calibrate LC2");
  Serial.println("3  - calibrate LC3");
  Serial.println("4  - calibrate LC4");
  Serial.println("p  - print scale factors");
  Serial.println("h  - show this menu");
  Serial.println();
}

void handleCalibrationSerial() {
  if (!Serial.available()) return;

  char cmd = Serial.read();

  switch (cmd) {
    case 't':
    case 'T':
      tareAllChannels(30);
      break;

    case 'c':
    case 'C':
      Serial.println("Live counts for 10 seconds...");
      for (uint16_t i = 0; i < 100; i++) {
        updateAllChannels();
        printCounts();
        delay(100);
      }
      break;

    case 'w':
    case 'W':
      Serial.println("Live weights for 10 seconds...");
      for (uint16_t i = 0; i < 100; i++) {
        updateAllChannels();
        printWeights();
        delay(100);
      }
      break;

    case '1':
    case '2':
    case '3':
    case '4': {
      uint8_t idx = cmd - '1';

      Serial.print("Enter known mass for LC");
      Serial.print(idx + 1);
      Serial.println(" in kg, then press Enter:");
//      Serial.println("Example: 1.906");

      float knownKg = readFloatFromSerial();

      Serial.print("You entered: ");
      Serial.println(knownKg, 3);

      if (knownKg <= 0.0f) {
        Serial.println("Invalid mass. Calibration cancelled.");
      } else {
        calibrateChannel(idx, knownKg);
      }
      break;
    }

    case 'p':
    case 'P':
      printScaleFactors();
      break;

    case 'h':
    case 'H':
      printCalibrationMenu();
      break;

    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  pinMode(PIN_MODESEL, INPUT_PULLUP);

  adsInit();

  for (uint8_t i = 0; i < 4; i++) {
    readChannelRaw(lc[i].pos, lc[i].neg);
    delay(50);
  }

  tareAllChannels(30);

  if (digitalRead(PIN_MODESEL) == LOW) {
    runMode = MODE_CALIBRATION;
    Serial.println("Boot mode: CALIBRATION");
    printCalibrationMenu();
  } else {
    runMode = MODE_OPERATION;
    Serial.println("Boot mode: OPERATION");
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