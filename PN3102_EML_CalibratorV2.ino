#include <Arduino_PortentaMachineControl.h>
#include <Wire.h>

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

static const uint8_t XIAO_ADDR = 0x2A;

OperationMode mode = MODE_IDLE;

//Load Cell Globals
float lcWeight[4] = {0};
//SPISettings adsSpiSettings(1000000, MSBFIRST, SPI_MODE1);

//  Voltage levels (0-10V) to control water flow / needlevalves
float voltageNV[4] = {0};

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

struct __attribute__((packed)) WeightPacket {
  float lc1;
  float lc2;
  float lc3;
  float lc4;
  uint32_t sequence;
};

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
//NeedleValves for delivering
//accurate flow to gauges
//****************************
void needleValve_controller(int needleValveNo){

  //Set the values of the voltages for the four needleValves??????

  //  Write the voltage values to the needleValves
  MachineControl_AnalogOut.write(needleValveNo, voltageNV[needleValveNo]);
  //MachineControl_AnalogOut.write(NEEDLE_VALVE_2, voltageNV1);
  //MachineControl_AnalogOut.write(NEEDLE_VALVE_3, voltageNV2);
  //MachineControl_AnalogOut.write(NEEDLE_VALVE_4, voltageNV3);

  // Voltage ramp TESTING !!!!!!!
  voltageNV[needleValveNo] += 0.1;
  delay(500);  // allow 10V -> 0V transition
  if (voltageNV[needleValveNo] >= 10.0) {
    voltageNV[needleValveNo] = 0;
    delay(500);  // allow 10V -> 0V transition
  }
}

void needleValve_reset(int needleValveNo){
  //  Write the voltage values to the needleValves
  voltageNV[needleValveNo] = 0.0;
  MachineControl_AnalogOut.write(needleValveNo, voltageNV[needleValveNo]);

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
    Serial.print(voltageNV[0]);
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

bool readLoadCells() {
  WeightPacket pkt = {0};

  Wire.requestFrom(XIAO_ADDR, (uint8_t)sizeof(pkt));

  uint8_t *p = (uint8_t *)&pkt;
  uint8_t i = 0;

  while (Wire.available() && i < sizeof(pkt)) {
    p[i++] = Wire.read();
  }

  if (i != sizeof(pkt)) {
    return false;
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
    Serial.print(pkt.lc4, 3);

    Serial.println();
  } else {
    Serial.print("I2C error, bytes read = ");
    Serial.println(i);
  }

  lcWeight[0] = pkt.lc1;
  lcWeight[1] = pkt.lc2;
  lcWeight[2] = pkt.lc3;
  lcWeight[3] = pkt.lc4;

  for (int i = 0; i < 4; i++) {
    if (abs(lcWeight[i]) < 0.005f) {
      lcWeight[i] = 0.0f;
    }
  }

  return true;
}


//*****************************************************************************************************
//SETUP
//*****************************************************************************************************
void setup() {
  // Initialize USB Serial
  Serial.begin(115200);
  delay(1000);   // allow USB to settle
  serialActive = Serial;  //Serial comms watchdog to prevent lock up

  Wire.begin();

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
          mode = MODE_IDLE;
          printMenu();
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

      for (int i = 0; i < 4; i++) {
        //Turn off all solenoids
        solenoid_controller(i, LOW);
        //Shut all Needle Valves
        needleValve_reset(i);
      }

      break;

    case MODE_SETUP:
        // FILL TANK 0
        // Check weight of load cell 0
        // if not full open solennoid 0
        readLoadCells();
        
        solenoid_controller(0, HIGH);
      break;

    case MODE_BALANCE:
      // balance code here
      needleValve_controller(0);
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
