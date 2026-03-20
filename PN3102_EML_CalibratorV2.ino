#include <Arduino_PortentaMachineControl.h>

//*****************************************************************************************************
//DEFINES
//*****************************************************************************************************
#define PERIOD_MS 4        // 4 ms = 250 Hz

// Solenoid switches (digital out) for on/off supply of water to the water containers
// NOTE: connect pin "24V IN" of the DIGITAL_OUTPUTS connector to 24V
#define SOLENOID_1 0
//#define SOLENOID_2 1
//#define SOLENOID_3 2
//#define SOLENOID_4 3

//Load Cells
#define ADS1256_CS_PIN     10
#define ADS1256_DRDY_PIN    9
#define ADS1256_RST_PIN     8   // optional, or tie high and ignore

// Needle Valves (analog out) for control of water flow
#define NEEDLE_VALVE_1 0
//#define NEEDLE_VALVE_2 1
//#define NEEDLE_VALVE_3 2
//#define NEEDLE_VALVE_4 3

//*****************************************************************************************************
//GLOBALS
//*****************************************************************************************************
// Define operation modes
enum OperationMode {
  MODE_IDLE = 1,
  MODE_SETUP,
  MODE_BALANCE,
  MODE_TEST,
  MODE_ERROR,
  MODE_SHUTDOWN
};

OperationMode mode = MODE_IDLE;

//Load Cell Globals

SPISettings adsSpiSettings(1000000, MSBFIRST, SPI_MODE1);

// ADS1256 Commands
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

// ADS1256 Registers
static const uint8_t REG_STATUS = 0x00;
static const uint8_t REG_MUX    = 0x01;
static const uint8_t REG_ADCON  = 0x02;
static const uint8_t REG_DRATE  = 0x03;
static const uint8_t REG_IO     = 0x04;
static const uint8_t REG_OFC0   = 0x05;
static const uint8_t REG_OFC1   = 0x06;
static const uint8_t REG_OFC2   = 0x07;
static const uint8_t REG_FSC0   = 0x08;
static const uint8_t REG_FSC1   = 0x09;
static const uint8_t REG_FSC2   = 0x0A;

// Common DRATE values from ADS1256 datasheet
static const uint8_t DRATE_1000SPS = 0xA1;
static const uint8_t DRATE_100SPS  = 0x82;
static const uint8_t DRATE_30SPS   = 0x53;
static const uint8_t DRATE_10SPS   = 0x23;

// ADS1256 PGA gain bits in ADCON
static const uint8_t PGA_1   = 0x00;
static const uint8_t PGA_2   = 0x01;
static const uint8_t PGA_4   = 0x02;
static const uint8_t PGA_8   = 0x03;
static const uint8_t PGA_16  = 0x04;
static const uint8_t PGA_32  = 0x05;
static const uint8_t PGA_64  = 0x06;


//  Voltage levels (0-10V) to control water flow / needlevalves
float voltageNV1 = 0;
//float voltageNV2 = 0;
//float voltageNV3 = 0;
//float voltageNV4 = 0;

// Raingauge channel counts
unsigned long pulseCountRG1A = 0;   // total pulses counted on RG1 chA
unsigned long pulseCountRG1B = 0;   // total pulses counted on RG1 chB
//unsigned long pulseCountRG2A = 0;   // total pulses counted on RG2 chA
//unsigned long pulseCountRG2B = 0;   // total pulses counted on RG2 chB
//unsigned long pulseCountRG3A = 0;   // total pulses counted on RG3 chA
//unsigned long pulseCountRG3B = 0;   // total pulses counted on RG3 chB
//unsigned long pulseCountRG4A = 0;   // total pulses counted on RG4 chA
//unsigned long pulseCountRG4B = 0;   // total pulses counted on RG4 chB
// Raingauge channel last states
bool lastPulseStateRG1A = LOW;     // last pin state for edge detection on RG1 chA
bool lastPulseStateRG1B = LOW;     // last pin state for edge detection on RG1 chB
//bool lastPulseStateRG2A = LOW;     // last pin state for edge detection on RG2 chA
//bool lastPulseStateRG2B = LOW;     // last pin state for edge detection on RG2 chB
//bool lastPulseStateRG3A = LOW;     // last pin state for edge detection on RG3 chA
//bool lastPulseStateRG3B = LOW;     // last pin state for edge detection on RG3 chB
//bool lastPulseStateRG4A = LOW;     // last pin state for edge detection on RG4 chA
//bool lastPulseStateRG4B = LOW;     // last pin state for edge detection on RG4 chB
// Raingauge channel current states
bool currentPulseStateRG1A = LOW;  // last pin state for edge detection on RG1 chA
bool currentPulseStateRG1B = LOW;  // last pin state for edge detection on RG1 chb
//bool currentPulseStateRG2A = LOW;  // last pin state for edge detection on RG2 chA
//bool currentPulseStateRG2B = LOW;  // last pin state for edge detection on RG2 chB
//bool currentPulseStateRG3A = LOW;  // last pin state for edge detection on RG3 chA
//bool currentPulseStateRG3B = LOW;  // last pin state for edge detection on RG3 chB
//bool currentPulseStateRG4A = LOW;  // last pin state for edge detection on RG4 chA
//bool currentPulseStateRG4B = LOW;  // last pin state for edge detection on RG4 chB

// Serial watchdog
bool serialActive = false;
unsigned long lastSerialCheck = 0;

// Heartbeat LED
unsigned long lastHeartbeat = 0;
bool heartbeatState = false;


//*****************************************************************************************************
//HELPERS
//*****************************************************************************************************

//****************************
//Solenoids for control of
//filling water tanks
//****************************
void solenoid_controller(uint8_t solenoid_no, PinStatus status){  //  Turns the solenoids On (HIGH) or Off (LOW)
  MachineControl_DigitalOutputs.write(solenoid_no, status);
}

//****************************
//Load Cells / ADS1256
// Low-level helpers
//****************************
void ads_cs_low()  { digitalWrite(ADS1256_CS_PIN, LOW); }
void ads_cs_high() { digitalWrite(ADS1256_CS_PIN, HIGH); }

void ads_sendCommand(uint8_t cmd) {
  SPI.beginTransaction(adsSpiSettings);
  ads_cs_low();
  SPI.transfer(cmd);
  ads_cs_high();
  SPI.endTransaction();
}

void ads_waitDRDY(uint32_t timeout_ms = 100) {
  uint32_t start = millis();
  while (digitalRead(ADS1256_DRDY_PIN) == HIGH) {
    if ((millis() - start) > timeout_ms) {
      break;
    }
  }
}

void ads_writeRegister(uint8_t reg, uint8_t value) {
  SPI.beginTransaction(adsSpiSettings);
  ads_cs_low();
  SPI.transfer(CMD_WREG | reg);
  SPI.transfer(0x00);     // write one register
  SPI.transfer(value);
  ads_cs_high();
  SPI.endTransaction();
  delayMicroseconds(10);
}

uint8_t ads_readRegister(uint8_t reg) {
  uint8_t value;
  SPI.beginTransaction(adsSpiSettings);
  ads_cs_low();
  SPI.transfer(CMD_RREG | reg);
  SPI.transfer(0x00);     // read one register
  delayMicroseconds(10);
  value = SPI.transfer(0xFF);
  ads_cs_high();
  SPI.endTransaction();
  return value;
}

long ads_readData() {
  uint8_t b0, b1, b2;
  long value;

  ads_waitDRDY();

  SPI.beginTransaction(adsSpiSettings);
  ads_cs_low();
  SPI.transfer(CMD_RDATA);
  delayMicroseconds(10);

  b0 = SPI.transfer(0xFF);
  b1 = SPI.transfer(0xFF);
  b2 = SPI.transfer(0xFF);

  ads_cs_high();
  SPI.endTransaction();

  value = ((long)b0 << 16) | ((long)b1 << 8) | b2;

  // Sign-extend 24-bit to 32-bit
  if (value & 0x800000) {
    value |= 0xFF000000;
  }

  return value;
}

void ads_setDifferentialChannel(uint8_t pos, uint8_t neg) {
  uint8_t mux = (pos << 4) | (neg & 0x0F);

  ads_writeRegister(REG_MUX, mux);

  // Sync and wakeup sequence after mux change
  SPI.beginTransaction(adsSpiSettings);
  ads_cs_low();
  SPI.transfer(CMD_SYNC);
  delayMicroseconds(4);
  SPI.transfer(CMD_WAKEUP);
  ads_cs_high();
  SPI.endTransaction();

  delayMicroseconds(10);
}

bool ads1256_begin() {
  pinMode(ADS1256_CS_PIN, OUTPUT);
  digitalWrite(ADS1256_CS_PIN, HIGH);

  pinMode(ADS1256_DRDY_PIN, INPUT_PULLUP);

  pinMode(ADS1256_RST_PIN, OUTPUT);
  digitalWrite(ADS1256_RST_PIN, HIGH);

  SPI.begin();

  // Hardware reset
  digitalWrite(ADS1256_RST_PIN, LOW);
  delay(5);
  digitalWrite(ADS1256_RST_PIN, HIGH);
  delay(5);

  // Stop continuous read mode just in case
  ads_sendCommand(CMD_SDATAC);
  delay(2);

  ads_waitDRDY();

  // STATUS: buffer off initially, auto-cal on
  ads_writeRegister(REG_STATUS, 0x04);

  // ADCON: clock out off, sensor detect off, PGA gain = 64
  ads_writeRegister(REG_ADCON, PGA_64);

  // Data rate: start with 30 SPS or 10 SPS for lower noise
  ads_writeRegister(REG_DRATE, DRATE_30SPS);

  delay(2);
  ads_sendCommand(CMD_SELFCAL);
  delay(5);

  return true;
}

struct LoadCellChannel {
  uint8_t ain_p;
  uint8_t ain_n;
  long raw;
  float filtered;
  long offset;
  float scale;      // kg per count
  float weightKg;
};

LoadCellChannel lc[4] = {
  {0, 1, 0, 0.0f, 0, 1.0f, 0.0f},
  {2, 3, 0, 0.0f, 0, 1.0f, 0.0f},
  {4, 5, 0, 0.0f, 0, 1.0f, 0.0f},
  {6, 7, 0, 0.0f, 0, 1.0f, 0.0f}
};

const float alpha = 0.1f;   // low-pass filter


void read_all_loadcells() {
  for (int i = 0; i < 4; i++) {
    ads_setDifferentialChannel(lc[i].ain_p, lc[i].ain_n);
    lc[i].raw = ads_readData();

    if (lc[i].filtered == 0.0f) {
      lc[i].filtered = (float)lc[i].raw;
    } else {
      lc[i].filtered = (1.0f - alpha) * lc[i].filtered + alpha * (float)lc[i].raw;
    }

    lc[i].weightKg = (lc[i].filtered - lc[i].offset) * lc[i].scale;
  }
}

void tare_all_loadcells(uint16_t samples = 20) {
  float sums[4] = {0, 0, 0, 0};

  for (uint16_t s = 0; s < samples; s++) {
    for (int i = 0; i < 4; i++) {
      ads_setDifferentialChannel(lc[i].ain_p, lc[i].ain_n);
      long v = ads_readData();
      sums[i] += (float)v;
    }
  }

  for (int i = 0; i < 4; i++) {
    lc[i].offset = (long)(sums[i] / samples);
    lc[i].filtered = (float)lc[i].offset;
  }
}

void calibrate_one_loadcell(uint8_t i, float knownKg) {
  // Call this after tare, with known mass on the selected cell
  ads_setDifferentialChannel(lc[i].ain_p, lc[i].ain_n);
  long raw = ads_readData();

  long delta = raw - lc[i].offset;
  if (delta != 0) {
    lc[i].scale = knownKg / (float)delta;
  }
}

void print_loadcells() {
  Serial.print("LC1: "); Serial.print(lc[0].weightKg, 3); Serial.print(" kg, ");
  Serial.print("LC2: "); Serial.print(lc[1].weightKg, 3); Serial.print(" kg, ");
  Serial.print("LC3: "); Serial.print(lc[2].weightKg, 3); Serial.print(" kg, ");
  Serial.print("LC4: "); Serial.print(lc[3].weightKg, 3); Serial.println(" kg");
}

//****************************
//NeedleValves for delivering
//accurate flow to gauges
//****************************
void needleValve_controller(void){

  //Set the values of the voltages for the four needleValves??????

  //  Write the voltage values to the needleValves
  MachineControl_AnalogOut.write(NEEDLE_VALVE_1, voltageNV1);
  //MachineControl_AnalogOut.write(NEEDLE_VALVE_2, voltageNV1);
  //MachineControl_AnalogOut.write(NEEDLE_VALVE_3, voltageNV2);
  //MachineControl_AnalogOut.write(NEEDLE_VALVE_4, voltageNV3);

  // Voltage ramp TESTING !!!!!!!
  voltageNV1 += 0.1;
  if (voltageNV1 >= 10.0) {
    voltageNV1 = 0;
    delay(100);  // allow 10V -> 0V transition
  }
}

//****************************
//Fuction for taking readings
//from the rain gauges
//****************************
void read_raingauges(void){
    //Read Raingauge 1A
  currentPulseStateRG1A = MachineControl_DigitalInputs.read(DIN_READ_CH_PIN_00);  //  Read Raingauge input 1A = DIN0
  if (lastPulseStateRG1A == HIGH && currentPulseStateRG1A == LOW) {
    pulseCountRG1A++;  // count falling edge
  }
  lastPulseStateRG1A = currentPulseStateRG1A;
  
  //Read Raingauge 1B
  currentPulseStateRG1B = MachineControl_DigitalInputs.read(DIN_READ_CH_PIN_01);  //  Read Raingauge input 1B = DIN1
  if (lastPulseStateRG1B == HIGH && currentPulseStateRG1B == LOW) {
    pulseCountRG1B++;  // count falling edge
  }
  lastPulseStateRG1B = currentPulseStateRG1B;
/*
  //Read Raingauge 2A
  currentPulseStateRG2A = MachineControl_DigitalInputs.read(DIN_READ_CH_PIN_02);  //  Read Raingauge input 2A = DIN2
  if (lastPulseStateRG2A == HIGH && currentPulseStateRG2A == LOW) {
    pulseCountRG2A++;  // count falling edge
  }
  lastPulseStateRG2A = currentPulseStateRG2A;
  
  //Read Raingauge 2B
  currentPulseStateRG2B = MachineControl_DigitalInputs.read(DIN_READ_CH_PIN_03);  //  Read Raingauge input 2B = DIN3
  if (lastPulseStateRG2B == HIGH && currentPulseStateRG2B == LOW) {
    pulseCountRG2B++;  // count falling edge
  }
  lastPulseStateRG2B = currentPulseStateRG2B;

  //Read Raingauge 3A
  currentPulseStateRG3A = MachineControl_DigitalInputs.read(DIN_READ_CH_PIN_04);  //  Read Raingauge input 3A = DIN4
  if (lastPulseStateRG3A == HIGH && currentPulseStateRG3A == LOW) {
    pulseCountRG3A++;  // count falling edge
  }
  lastPulseStateRG3A = currentPulseStateRG3A;
  
  //Read Raingauge 3B
  currentPulseStateRG3B = MachineControl_DigitalInputs.read(DIN_READ_CH_PIN_05);  //  Read Raingauge input 3B = DIN5
  if (lastPulseStateRG3B == HIGH && currentPulseStateRG3B == LOW) {
    pulseCountRG3B++;  // count falling edge
  }
  lastPulseStateRG3B = currentPulseStateRG3B;


  //Read Raingauge 4A
  currentPulseStateRG4A = MachineControl_DigitalInputs.read(DIN_READ_CH_PIN_06);  //  Read Raingauge input 4A = DIN6
  if (lastPulseStateRG4A == HIGH && currentPulseStateRG4A == LOW) {
    pulseCountRG4A++;  // count falling edge
  }
  lastPulseStateRG4A = currentPulseStateRG4A;
  
  //Read Raingauge 4B
  currentPulseStateRG4B = MachineControl_DigitalInputs.read(DIN_READ_CH_PIN_07);  //  Read Raingauge input 4B = DIN7
  if (lastPulseStateRG4B == HIGH && currentPulseStateRG4B == LOW) {
    pulseCountRG4B++;  // count falling edge
  }
  lastPulseStateRG4B = currentPulseStateRG4B;
  */
}

//****************************
//Test serial outputs
//****************************
void serial_debug(void){
  // Re-check Serial every second
  if (millis() - lastSerialCheck > 1000) {
    serialActive = Serial;
    lastSerialCheck = millis();
  }
  // Print values if Serial is connected
  if (serialActive) {
    Serial.print("NV1: ");
    Serial.print(voltageNV1);
    Serial.print(" V, RG1A: ");
    Serial.print(pulseCountRG1A);
    Serial.print(" , RG1B: ");
    Serial.println(pulseCountRG1B);
/*
    Serial.print("NV2: ");
    Serial.print(voltageNV2);
    Serial.print(" V, RG2A: ");
    Serial.print(pulseCountRG2A);
    Serial.print(" , RG2B: ");
    Serial.println(pulseCountRG2B);

    Serial.print("NV3: ");
    Serial.print(voltageNV3);
    Serial.print(" V, RG3A: ");
    Serial.print(pulseCountRG3A);
    Serial.print(" , RG3B: ");
    Serial.println(pulseCountRG3B);

    Serial.print("NV4: ");
    Serial.print(voltageNV4);
    Serial.print(" V, RG4A: ");
    Serial.print(pulseCountRG4A);
    Serial.print(" , RG4B: ");
    Serial.println(pulseCountRG4B);
    */
  }
}

//****************************
//Device heatbeat status
//****************************
void led_heartbeat(void){
    // Heartbeat LED: toggle every 500 ms
  if (millis() - lastHeartbeat > 500) {
    heartbeatState = !heartbeatState;
    MachineControl_DigitalProgrammables.set(IO_WRITE_CH_PIN_00, heartbeatState);
    lastHeartbeat = millis();
  }
}

//*****************************************************************************************************
//SETUP
//*****************************************************************************************************
void setup() {
  // Initialize USB Serial
  Serial.begin(115200);
  delay(1000);   // allow USB to settle
  serialActive = Serial;  //Serial comms watchdog to prevent lock up

  if (serialActive) {
    Serial.println("*********EML Calibrator*********");
  }

  //Set over current behavior of all solenoid digital channels to latch mode (true)
  MachineControl_DigitalOutputs.begin(true);
  //At startup set all solenoid digital channels to OPEN
  MachineControl_DigitalOutputs.writeAll(0);

  Wire.begin();

  if (!MachineControl_DigitalProgrammables.begin()) {
    Serial.println("GPIO expander initialization fail!!");
  }

  if (!MachineControl_DigitalInputs.begin()) {
    Serial.println("Digital input GPIO expander initialization fail!!");
 }

  // Initialize analog outputs - Needle Valves
  MachineControl_AnalogOut.begin();
  MachineControl_AnalogOut.setPeriod(NEEDLE_VALVE_1, PERIOD_MS);
//  MachineControl_AnalogOut.setPeriod(NEEDLE_VALVE_2, PERIOD_MS);
//  MachineControl_AnalogOut.setPeriod(NEEDLE_VALVE_3, PERIOD_MS);
//  MachineControl_AnalogOut.setPeriod(NEEDLE_VALVE_4, PERIOD_MS);

  // Initialize digtal outputs - Solenoids
  solenoid_controller(SOLENOID_1, LOW);  // Turn off solenoid 1
//  solenoid_controller(SOLENOID_2, LOW);  // Turn off solenoid 2
//  solenoid_controller(SOLENOID_3, LOW);  // Turn off solenoid 3
//  solenoid_controller(SOLENOID_4, LOW);  // Turn off solenoid 4

  delay(1000);  // allow setups to settle 

  Serial.println("Enter a number to choose a mode:");
  Serial.println("1. IDLE");
  Serial.println("2. SETUP");
  Serial.println("3. BALANCE");
  Serial.println("4. TEST");  
  Serial.println("5. ERROR");
  Serial.println("6. SHUTDOWN");

}

void printMenu() {
  Serial.println("\nEnter a number to choose a mode:");
  Serial.println("1. IDLE");
  Serial.println("2. SETUP");
  Serial.println("3. BALANCE");
  Serial.println("4. TEST");
  Serial.println("5. ERROR");
  Serial.println("6. SHUTDOWN");
  Serial.println("0. Exit current mode / return to menu");
}

//*****************************************************************************************************
//LOOP
//*****************************************************************************************************
void loop() {

  // Always check serial input
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    // Ignore newline / carriage return
    if (cmd == '\n' || cmd == '\r') {
      return;
    }

    switch (cmd) {
      case '1':
        mode = MODE_IDLE;
        Serial.println("System is in IDLE mode.");
        printMenu();
        break;

      case '2':
        mode = MODE_SETUP;
        Serial.println("System is in SETUP.");
        Serial.println("Press 0 to return to menu.");
        break;

      case '3':
        mode = MODE_BALANCE;
        Serial.println("System is in BALANCE TEST.");
        printMenu();
        break;

      case '4':
        mode = MODE_TEST;
        Serial.println("System is in mm/hr TEST.");
        printMenu();
        break;

      case '5':
        mode = MODE_ERROR;
        Serial.println("System encountered an ERROR!");
        printMenu();
        break;

      case '6':
        mode = MODE_SHUTDOWN;
        Serial.println("System is SHUTTING DOWN.");
        while (true) {
          delay(1000);
        }
        break;

      case '0':
        if (mode == MODE_SETUP) {
          mode = MODE_IDLE;
          Serial.println("Exiting SETUP.");
          printMenu();
        }
        break;

      default:
        Serial.println("Invalid choice!");
        printMenu();
        break;
    }
  }

  // Run active mode
  switch (mode) {

    case MODE_IDLE:
        //Turn off solenoids
        solenoid_controller(0, LOW);
      break;

    case MODE_SETUP:
        // FILL TANK 0
        // Check weight of load cell 0
        // if not full open solennoid 0
        solenoid_controller(0, HIGH);
      break;

    case MODE_BALANCE:
      // balance code here
      break;

    case MODE_TEST:
      // test code here
      break;

    case MODE_ERROR:
    default:
      break;
  }

  delay(10);
}
