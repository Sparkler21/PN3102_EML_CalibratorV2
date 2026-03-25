#include <Arduino_PortentaMachineControl.h>
#include <PortentaEthernet.h>
#include <Ethernet.h>
#include <Wire.h>

//*****************************************************************************************************
//DEFINES
//*****************************************************************************************************
#define PERIOD_MS 4        // 4 ms = 250 Hz

//*****************************************************************************************************
// Ethernet config
//*****************************************************************************************************
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Pick an address that fits your LAN
IPAddress ip(172, 16, 168, 99);
IPAddress dns(172, 16, 168, 252);
IPAddress gateway(172, 16, 168, 254);
IPAddress subnet(255, 255, 255, 0);

EthernetServer server(5000);
EthernetClient client;

//*****************************************************************************************************
//GLOBALS
//*****************************************************************************************************

//*****************************************************************************************************
//Core Enums and Structs
//*****************************************************************************************************
// Define operation modes
enum OperationMode {
  MODE_IDLE = 0,
  MODE_SETUP,
  MODE_BALANCE,
  MODE_TEST,
  MODE_ERROR,
  MODE_SHUTDOWN
};
OperationMode mode = MODE_IDLE;

enum RigMode {
  RIG_IDLE = 0,
  RIG_FILL,
  RIG_SETTLE,
  RIG_STABILIZE,
  RIG_RUN,
  RIG_FINISH,
  RIG_COMPLETE,
  RIG_ERROR
};

struct RigState {
  RigMode mode;
  uint32_t stateEntryMs;
  uint32_t stableSinceMs;
  int fault;
  bool enabled;
  bool startRequest;
  bool stopRequest;
};

struct RigData {
  float massKg;
  float filteredMassKg;
  float lastMassKg;
  float flowLpm;
  float valveCommandV;
  bool fillSolenoidOn;
  bool sensorOk;
};

struct RigRecipe {
  float startMassKg;
  float targetFlowLpm;
  float settleBandKg;
  float flowBandLpm;
  uint32_t fillTimeoutMs;
  uint32_t settleTimeoutMs;
  uint32_t stabilizeTimeoutMs;
  uint32_t runDurationMs;
};

RigState rigState[4];
RigData rigData[4];
RigRecipe rigRecipe[4];

enum CommandType {
  CMD_UNKNOWN = 0,
  CMD_PING,
  CMD_GET_STATUS,
  CMD_GET_WEIGHTS,
  CMD_GET_WEIGHT,
  CMD_SET_VALVE,
  CMD_GET_VALVE,
  CMD_FILL_TANK,
  CMD_STOP_TANK,
  CMD_STOP_ALL
};

struct ParsedCommand {
  CommandType type = CMD_UNKNOWN;
  int channel = -1;       // 1..4 from protocol
  float value = 0.0f;     // e.g. valve voltage
  bool valid = false;
  String error = "";
};

//Load Cell Globals
static const uint8_t XIAO_ADDR = 0x2A;
float lcWeight[4] = {0};
uint32_t lastSeq = 0;

struct __attribute__((packed)) WeightPacket {
  float lc1;
  float lc2;
  float lc3;
  float lc4;
  uint32_t sequence;
};

//  Voltage levels (0-10V) to control water flow / needlevalves
float voltageNV[4] = {0};
float valveSetpoint[4] = {0.0f, 0.0f, 0.0f, 0.0f};

// Raingauge channel counts
unsigned long pulseCountRGA[4] = {0};   // total pulses counted on RG chA
unsigned long pulseCountRGB[4] = {0};   // total pulses counted on RG chB
// Raingauge channel last states
bool lastPulseStateRGA[4] = {LOW};     // last pin state for edge detection on RG chA
bool lastPulseStateRGB[4] = {LOW};     // last pin state for edge detection on RG chB
// Raingauge channel current states
bool currentPulseStateRGA[4] = {LOW};  // last pin state for edge detection on RG chA
bool currentPulseStateRGB[4] = {LOW};  // last pin state for edge detection on RG chb

// Serial watchdog
bool serialActive = false;
unsigned long lastSerialCheck = 0;

// Heartbeat LED
unsigned long lastHeartbeat = 0;
bool heartbeatState = false;

//*****************************************************************************************************
//HELPERS
//*****************************************************************************************************

//***********************************************************
// Main Menu
//***********************************************************
void printMenu() {
  Serial.println("\nEnter a number to choose a mode:");
  Serial.println("0. IDLE / Exit current mode");
  Serial.println("1. SETUP");
  Serial.println("2. BALANCE");
  Serial.println("3. TEST");
  Serial.println("4. ERROR");
  Serial.println("5. SHUTDOWN");
}

//***********************************************************
// Parsing Helper Functions
//***********************************************************
bool isIntegerString(const String &s) {
  if (s.length() == 0) return false;

  for (unsigned int i = 0; i < s.length(); i++) {
    if (!isDigit(s[i])) {
      return false;
    }
  }
  return true;
}

bool isFloatString(const String &s) {
  if (s.length() == 0) return false;

  bool decimalFound = false;

  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];

    if (c == '.') {
      if (decimalFound) return false;
      decimalFound = true;
    }
    else if (!isDigit(c)) {
      return false;
    }
  }
  return true;
}

bool isValidChannel(int channel) {
  return (channel >= 1 && channel <= 4);
}

// Split by comma into up to maxParts entries.
// Returns actual number of parts found.
int splitCsv(const String &input, String parts[], int maxParts) {
  int count = 0;
  int start = 0;

  while (count < maxParts) {
    int comma = input.indexOf(',', start);

    if (comma == -1) {
      parts[count] = input.substring(start);
      parts[count].trim();
      count++;
      break;
    }

    parts[count] = input.substring(start, comma);
    parts[count].trim();
    count++;

    start = comma + 1;
  }

  return count;
}

//***********************************************************
// Parsing Functions
//***********************************************************
ParsedCommand parseCommand(const String &rawCmd) {
  ParsedCommand pc;
  String cmd = rawCmd;
  cmd.trim();

  if (cmd.length() == 0) {
    pc.error = "EMPTY_COMMAND";
    return pc;
  }

  // Single-word commands
  if (cmd.equalsIgnoreCase("PING")) {
    pc.type = CMD_PING;
    pc.valid = true;
    return pc;
  }

  if (cmd.equalsIgnoreCase("GET_STATUS")) {
    pc.type = CMD_GET_STATUS;
    pc.valid = true;
    return pc;
  }

  if (cmd.equalsIgnoreCase("GET_WEIGHTS")) {
    pc.type = CMD_GET_WEIGHTS;
    pc.valid = true;
    return pc;
  }

  if (cmd.equalsIgnoreCase("STOP_ALL")) {
    pc.type = CMD_STOP_ALL;
    pc.valid = true;
    return pc;
  }

  // CSV commands
  String parts[3];
  int partCount = splitCsv(cmd, parts, 3);

  if (partCount <= 0) {
    pc.error = "BAD_FORMAT";
    return pc;
  }

  // GET_WEIGHT,<channel>
  if (parts[0].equalsIgnoreCase("GET_WEIGHT")) {
    if (partCount != 2) {
      pc.error = "BAD_FORMAT";
      return pc;
    }

    if (!isIntegerString(parts[1])) {
      pc.error = "BAD_CHANNEL";
      return pc;
    }

    pc.channel = parts[1].toInt();
    if (!isValidChannel(pc.channel)) {
      pc.error = "BAD_CHANNEL";
      return pc;
    }

    pc.type = CMD_GET_WEIGHT;
    pc.valid = true;
    return pc;
  }

  // GET_VALVE,<channel>
  if (parts[0].equalsIgnoreCase("GET_VALVE")) {
    if (partCount != 2) {
      pc.error = "BAD_FORMAT";
      return pc;
    }

    if (!isIntegerString(parts[1])) {
      pc.error = "BAD_CHANNEL";
      return pc;
    }

    pc.channel = parts[1].toInt();
    if (!isValidChannel(pc.channel)) {
      pc.error = "BAD_CHANNEL";
      return pc;
    }

    pc.type = CMD_GET_VALVE;
    pc.valid = true;
    return pc;
  }

  // FILL_TANK,<channel>
  if (parts[0].equalsIgnoreCase("FILL_TANK")) {
    if (partCount != 2) {
      pc.error = "BAD_FORMAT";
      return pc;
    }

    if (!isIntegerString(parts[1])) {
      pc.error = "BAD_CHANNEL";
      return pc;
    }

    pc.channel = parts[1].toInt();
    if (!isValidChannel(pc.channel)) {
      pc.error = "BAD_CHANNEL";
      return pc;
    }

    pc.type = CMD_FILL_TANK;
    pc.valid = true;
    return pc;
  }

  // STOP_TANK,<channel>
  if (parts[0].equalsIgnoreCase("STOP_TANK")) {
    if (partCount != 2) {
      pc.error = "BAD_FORMAT";
      return pc;
    }

    if (!isIntegerString(parts[1])) {
      pc.error = "BAD_CHANNEL";
      return pc;
    }

    pc.channel = parts[1].toInt();
    if (!isValidChannel(pc.channel)) {
      pc.error = "BAD_CHANNEL";
      return pc;
    }

    pc.type = CMD_STOP_TANK;
    pc.valid = true;
    return pc;
  }

  // SET_VALVE,<channel>,<value>
  if (parts[0].equalsIgnoreCase("SET_VALVE")) {
    if (partCount != 3) {
      pc.error = "BAD_FORMAT";
      return pc;
    }

    if (!isIntegerString(parts[1])) {
      pc.error = "BAD_CHANNEL";
      return pc;
    }

    if (!isFloatString(parts[2])) {
      pc.error = "BAD_VALUE";
      return pc;
    }

    pc.channel = parts[1].toInt();
    if (!isValidChannel(pc.channel)) {
      pc.error = "BAD_CHANNEL";
      return pc;
    }

    pc.value = parts[2].toFloat();
    if (pc.value < 0.0f || pc.value > 10.0f) {
      pc.error = "BAD_VALUE";
      return pc;
    }

    pc.type = CMD_SET_VALVE;
    pc.valid = true;
    return pc;
  }

  pc.error = "UNKNOWN_COMMAND";
  return pc;
}

//***********************************************************
// Execute Command Function
//***********************************************************
String executeCommand(const ParsedCommand &pc) {
  if (!pc.valid) {
    return "ERROR," + pc.error;
  }

  switch (pc.type) {
    case CMD_PING:
      return "PONG";

    case CMD_GET_STATUS:
      return "STATUS,RUN,0,1,IDLE,IDLE,IDLE,IDLE";

    case CMD_GET_WEIGHTS: {
      String s = "WEIGHTS,";
      s += String(lcWeight[0], 3) + ",";
      s += String(lcWeight[1], 3) + ",";
      s += String(lcWeight[2], 3) + ",";
      s += String(lcWeight[3], 3) + ",";
      s += String(lastSeq);
      return s;
    }

    case CMD_GET_WEIGHT: {
      int idx = pc.channel - 1;
      return "WEIGHT," + String(pc.channel) + "," + String(lcWeight[idx], 3);
    }

    case CMD_SET_VALVE: {
      int idx = pc.channel - 1;
      needleValve_controller(idx, pc.value);
      return "OK,SET_VALVE," + String(pc.channel) + "," + String(pc.value, 3);
    }

    case CMD_GET_VALVE: {
      int idx = pc.channel - 1;
      // Replace valveSetpoint[] with your real stored valve command array
      return "VALVE," + String(pc.channel) + "," + String(valveSetpoint[idx], 3);
    }

    case CMD_FILL_TANK: {
      int idx = pc.channel - 1;
      solenoid_controller(idx, HIGH);
      return "OK,FILL_TANK," + String(pc.channel);
    }

    case CMD_STOP_TANK: {
      int idx = pc.channel - 1;
      solenoid_controller(idx, LOW);
      return "OK,STOP_TANK," + String(pc.channel);
    }

    case CMD_STOP_ALL: {
      for (int i = 0; i < 4; i++) {
        solenoid_controller(i, LOW);
        needleValve_controller(i, 0.0f);
      }
      return "OK,STOP_ALL";
    }

    default:
      return "ERROR,UNKNOWN_COMMAND";
  }
}

//***********************************************************
// Ethernet Active
//***********************************************************
void ethernetClientActive(void){
  // Accept a client if none connected
  if (!client || !client.connected()) {
    EthernetClient newClient = server.available();
    if (newClient) {
      client = newClient;
      Serial.println("Client connected");
      client.println("OK,CONNECTED");
    }
  } else {
    String cmd = readLine(client);

    if (cmd.length() > 0) {
      Serial.print("RX: ");
      Serial.println(cmd);

      String reply = handleCommand(cmd);

      Serial.print("TX: ");
      Serial.println(reply);

      client.println(reply);
    }

    if (!client.connected()) {
      client.stop();
      Serial.println("Client disconnected");
    }
  }

}

//*****************************************************************************************************
// Ethernet: Read one line from TCP
//*****************************************************************************************************
String readLine(EthernetClient &c) {
  static String line = "";

  while (c.available()) {
    char ch = c.read();

    if (ch == '\r') continue;

    if (ch == '\n') {
      String out = line;
      line = "";
      out.trim();
      return out;
    }

    line += ch;

    // prevent runaway line length
    if (line.length() > 150) {
      line = "";
      return "ERROR_LINE_TOO_LONG";
    }
  }

  return "";
}

//*****************************************************************************************************
// Ethernet: Command handler
//*****************************************************************************************************
String handleCommand(const String &cmd) {
  ParsedCommand pc = parseCommand(cmd);
  return executeCommand(pc);
}

//***********************************************************
// Device - Solenoids / Tank Fill
//***********************************************************
void solenoid_controller(uint8_t solenoid_no, PinStatus status){  //  Turns the solenoids On (HIGH) or Off (LOW)
  MachineControl_DigitalOutputs.write(solenoid_no, status);
}

//***********************************************************
// Device - Needle Valves / Flow Control
//***********************************************************
void needleValve_reset(int needleValveNo){
  //  Write the voltage values to the needleValves
  voltageNV[needleValveNo] = 0.0;
  MachineControl_AnalogOut.write(needleValveNo, voltageNV[needleValveNo]);

}

void needleValve_controller(int needleValveNo, float volts) {
  if (needleValveNo < 0 || needleValveNo > 3) return;

  valveSetpoint[needleValveNo] = volts;

  //  Write the voltage values to the needleValves
  MachineControl_AnalogOut.write(needleValveNo, volts);
}

//***********************************************************
// Device - rain gauges
//***********************************************************
void read_raingauges(void){
    //Read Raingauge 1A
  currentPulseStateRGA[0] = MachineControl_DigitalInputs.read(DIN_READ_CH_PIN_00);  //  Read Raingauge input 1A = DIN0
  if (lastPulseStateRGA[0] == HIGH && currentPulseStateRGA[0] == LOW) {
    pulseCountRGA[0]++;  // count falling edge
  }
  lastPulseStateRGA[0] = currentPulseStateRGA[0];
  
  //Read Raingauge 1B
  currentPulseStateRGB[0] = MachineControl_DigitalInputs.read(DIN_READ_CH_PIN_01);  //  Read Raingauge input 1B = DIN1
  if (lastPulseStateRGB[0] == HIGH && currentPulseStateRGB[0] == LOW) {
    pulseCountRGB[0]++;  // count falling edge
  }
  lastPulseStateRGB[0] = currentPulseStateRGB[0];
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

//***********************************************************
//Test serial outputs
//***********************************************************
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
    Serial.print(pulseCountRGA[0]);
    Serial.print(" , RG1B: ");
    Serial.println(pulseCountRGB[0]);
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

//***********************************************************
//Device - Heatbeat Status
//***********************************************************
void led_heartbeat(void){
    // Heartbeat LED: toggle every 500 ms
  if (millis() - lastHeartbeat > 500) {
    heartbeatState = !heartbeatState;
    MachineControl_DigitalProgrammables.set(IO_WRITE_CH_PIN_00, heartbeatState);
    lastHeartbeat = millis();
  }
}
//***********************************************************
//Device - Load Cell Controls
//***********************************************************
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
  lastSeq = pkt.sequence;

  for (int i = 0; i < 4; i++) {
    if (abs(lcWeight[i]) < 0.005f) {
      lcWeight[i] = 0.0f;
    }
  }

  return true;
}

//***********************************************************
//  Read the selected MODE from serial
//***********************************************************
void readSelectedSerialMode(void){

  // Always check serial input
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    // Ignore newline / carriage return
    if (cmd == '\n' || cmd == '\r') {
      return;
    }

    switch (cmd) {
      case '0':
        mode = MODE_IDLE;
        Serial.println("System is in IDLE mode.");
        printMenu();
        break;

      case '1':
        mode = MODE_SETUP;
        Serial.println("System is in SETUP.");
        Serial.println("Press 0 to return to menu.");
        break;

      case '2':
        mode = MODE_BALANCE;
        Serial.println("System is in BALANCE TEST.");
        printMenu();
        break;

      case '3':
        mode = MODE_TEST;
        Serial.println("System is in mm/hr TEST.");
        printMenu();
        break;

      case '4':
        mode = MODE_ERROR;
        Serial.println("System encountered an ERROR!");
        printMenu();
        break;

      case '5':
        mode = MODE_SHUTDOWN;
        Serial.println("System is SHUTTING DOWN.");
        while (true) {
          delay(1000);
        }
        break;

//      case '0':
//          mode = MODE_IDLE;
//          printMenu();
//        break;

      default:
        Serial.println("Invalid choice!");
        printMenu();
        break;
    }
  }
}
//***********************************************************
//  Run the selected MODE
//***********************************************************
void runActiveMode(void){
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
      needleValve_controller(0,3);
      break;

    case MODE_TEST:
      // test code here
      break;

    case MODE_ERROR:
    default:
      break;
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

  Wire.begin();

  // Bring up Ethernet with a static IP
  Ethernet.begin(mac, ip, dns, gateway, subnet);
  delay(1000);

  server.begin();

  Serial.println("Portenta TCP server starting...");
  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());
  Serial.println("TCP port: 5000");

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
  for (int nv = 0; nv < 4; nv++) {
    MachineControl_AnalogOut.setPeriod(nv, PERIOD_MS);  // Set period (ms)
  }
  // Initialize digtal outputs - Solenoids
  for (int sd = 0; sd < 4; sd++) {
    solenoid_controller(sd, LOW);  // Turn off solenoid 1 to 4
  }

  delay(1000);  // allow setups to settle 
  printMenu();
}

//*****************************************************************************************************
//LOOP
//*****************************************************************************************************
void loop() {

  readSelectedSerialMode();
  runActiveMode();

  ethernetClientActive();
  delay(10);
}
