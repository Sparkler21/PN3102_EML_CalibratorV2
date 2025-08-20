/*

# Overview

This package gives you a working baseline for a **factory HMI** controlling your **water calibration machine** with an Arduino **Portenta Machine Control (PMC)**. It includes:

1. A **Modbus TCP register map** tailored to your machine.
2. A **Portenta firmware skeleton** (C++) exposing the registers and a safe state machine (IDLE, FILL, BALANCE\_TEST, TEST, SHUTDOWN).
3. A **Node-RED Dashboard starter flow** (JSON) that reads & writes those registers for an operator GUI.
4. A short **bring‑up checklist**.

---

## Assumptions (adjust as needed)

* **Connectivity**: Ethernet; PMC static IP `192.168.1.50`, Modbus TCP server on port `502`.
* **Actuators**:

  * 24 V DC **fill solenoid** (on/off) driven by PMC digital output (through relay/driver as appropriate).
  * **Needle valve** with 0–10 V analog control driven by PMC analog output (0–10 V range).
* **Sensing**:

  * **Load cell** → **ADS1234** → digital mass reading (grams). The firmware polls ADS1234.
* **Control**:

  * Flow is inferred from **dM/dt** of the load‑cell mass.
  * PID on flow is applied to the 0–10 V valve command during **BALANCE\_TEST/TEST**.
* **Safety**:

  * Core interlocks managed on the controller (GUI only requests actions). If you have an E‑Stop, wire it into a fail‑safe circuit; optionally read its state to `DI`.

---

## Modbus TCP Register Map (v1.0)

**Scaling rules**

* `x10` means value = register / 10 in engineering units.
* `x100` means value = register / 100 in engineering units.
* 32‑bit values use **two input registers**: **LO (low‑word) then HI (high‑word)**, signed if noted. Combine as: `int32 = (HI<<16)|LO` (treat as signed when needed).

### Coils (Write — Commands)

| Addr | Name                     | Description                                                                 |
| ---: | ------------------------ | --------------------------------------------------------------------------- |
|    0 | CMD\_START               | Start sequence (edge‑triggered; controller latches & auto‑clears).          |
|    1 | CMD\_STOP                | Normal stop (ramps down, safe state).                                       |
|    2 | CMD\_RESET\_FAULTS       | Clear fault & return to IDLE if safe.                                       |
|    3 | CMD\_ABORT               | Immediate abort to SHUTDOWN (non‑latching).                                 |
|    4 | CMD\_MODE\_IDLE          | Request IDLE mode.                                                          |
|    5 | CMD\_MODE\_FILL          | Request FILL mode.                                                          |
|    6 | CMD\_MODE\_BALANCE\_TEST | Request BALANCE\_TEST mode.                                                 |
|    7 | CMD\_MODE\_TEST          | Request TEST mode.                                                          |
|    8 | CMD\_MODE\_SHUTDOWN      | Request SHUTDOWN mode.                                                      |
|    9 | CMD\_MANUAL\_SOLENOID    | Manual override: 1=ON, 0=OFF (only honored in Manual/Service when enabled). |
|   10 | CMD\_TARE\_SCALE         | Perform load‑cell tare/zero.                                                |
|   11 | CMD\_ENABLE\_REMOTE      | Allow remote writes for 5 min (auto‑timeout).                               |

### Discrete Inputs (Read — Status Bits)

| Addr | Name                    | Description                                 |
| ---: | ----------------------- | ------------------------------------------- |
|    0 | ST\_ESTOP\_OK           | 1 if E‑Stop chain healthy (optional input). |
|    1 | ST\_FAULT\_ACTIVE       | Any fault latched.                          |
|    2 | ST\_REMOTE\_ENABLED     | Remote write window active.                 |
|    3 | ST\_MODE\_IDLE          | Current mode = IDLE.                        |
|    4 | ST\_MODE\_FILL          | Current mode = FILL.                        |
|    5 | ST\_MODE\_BALANCE\_TEST | Current mode = BALANCE\_TEST.               |
|    6 | ST\_MODE\_TEST          | Current mode = TEST.                        |
|    7 | ST\_MODE\_SHUTDOWN      | Current mode = SHUTDOWN.                    |
|    8 | ST\_SOLENOID\_ON        | Physical output state to the fill valve.    |
|    9 | ST\_VALVE\_ENABLED      | 0–10 V path armed/enabled.                  |
|   10 | ST\_SCALE\_OK           | ADS1234 reading valid & in range.           |
|   11 | ST\_HEARTBEAT\_OK       | Heartbeat echo matches recent sequence.     |

### Holding Registers (Write — Setpoints & Config)

| Addr | Name                  | Units / Scaling | Notes                                               |
| ---: | --------------------- | --------------- | --------------------------------------------------- |
|    0 | SP\_TARGET\_MASS      | grams x10       | Fill target mass (e.g., 12500 = 1250.0 g).          |
|    1 | SP\_FILL\_TIMEOUT     | s               | Max seconds in FILL before timeout fault.           |
|    2 | SP\_FLOW\_TARGET      | g/s x10         | Desired flow during balance/test.                   |
|    3 | SP\_FLOW\_KP          | x100            | PID P gain.                                         |
|    4 | SP\_FLOW\_KI          | x100            | PID I gain.                                         |
|    5 | SP\_FLOW\_KD          | x100            | PID D gain.                                         |
|    6 | SP\_VALVE\_MIN\_MV    | mV              | Lower clamp for 0–10 V (0–10000).                   |
|    7 | SP\_VALVE\_MAX\_MV    | mV              | Upper clamp for 0–10 V.                             |
|    8 | SP\_BALANCE\_DURATION | s               | How long to hold stable flow before TEST.           |
|    9 | SP\_TEST\_DURATION    | s               | Duration of TEST phase.                             |
|   10 | SP\_DERIV\_WINDOW     | ms              | Window for dM/dt filter (e.g., 500 ms).             |
|   11 | SP\_FLOW\_HYST        | g/s x10         | Deadband around flow target.                        |
|   12 | SP\_LOG\_INTERVAL     | ms              | 0=off; logging interval on PC side (advisory).      |
|  500 | CMD\_SEQ              | —               | Heartbeat sequence written by HMI (echoed to IR20). |

### Input Registers (Read — Telemetry)

| Addr | Name                    | Units / Scaling  | Notes                                                       |
| ---: | ----------------------- | ---------------- | ----------------------------------------------------------- |
|    0 | MASS\_G\_X10\_LO        | g x10            | 32‑bit low word.                                            |
|    1 | MASS\_G\_X10\_HI        | g x10            | 32‑bit high word.                                           |
|    2 | FLOW\_GPS\_X10\_LO      | g/s x10 (signed) | 32‑bit low word.                                            |
|    3 | FLOW\_GPS\_X10\_HI      | g/s x10 (signed) | 32‑bit high word.                                           |
|    4 | VALVE\_OUT\_MV          | mV               | Actual output to valve (0–10000).                           |
|    5 | STATE\_CODE             | —                | 0=IDLE,1=FILL,2=BALANCE\_TEST,3=TEST,4=SHUTDOWN,100+=FAULT. |
|    6 | LAST\_FAULT\_CODE       | —                | 0=none; see table below.                                    |
|    7 | SCALE\_RAW\_LO          | counts           | Optional 32‑bit raw ADC count low.                          |
|    8 | SCALE\_RAW\_HI          | counts           | Optional 32‑bit high.                                       |
|    9 | LOOP\_OUT\_PCT\_X10     | % x10            | PID output after clamps.                                    |
|   10 | LOOP\_ERR\_GPS\_X10\_LO | g/s x10          | 32‑bit low (signed).                                        |
|   11 | LOOP\_ERR\_GPS\_X10\_HI | g/s x10          | 32‑bit high (signed).                                       |
|   12 | TEMP\_C\_X10            | °C x10           | Optional board temp if available.                           |
|   20 | HB\_ECHO                | —                | Echo of `CMD_SEQ` written by HMI.                           |

### Fault Codes (suggested)

| Code | Meaning                        |
| ---: | ------------------------------ |
|    0 | No fault                       |
|  101 | E‑Stop not OK                  |
|  102 | Scale not ready / out of range |
|  103 | Fill timeout                   |
|  104 | Valve command out of limits    |
|  105 | Communication/heartbeat lost   |
|  199 | Generic unexpected state       |



**Note:** The PID → mV mapping is deliberately simple. You’ll tune the scale so that controller output maps sensibly into 0–10 V for your valve. Start with low gains.

*/

## Portenta Firmware Skeleton (C++)

// Drop this into the Arduino IDE (choose Portenta H7 + Machine Control libs). This is a **template** — fill in hardware pin mappings and ADS1234 driver specifics where marked.

#include <Arduino.h>
#include <ArduinoModbus.h>
#include <Ethernet.h>

// ==== Networking ====
byte mac[] = {0xDE,0xAD,0xBE,0xEF,0xFE,0xED};
IPAddress ip(192,168,1,50);

// ==== I/O Mappings (TODO: adjust to your board wiring) ====
const int PIN_SOLENOID = 2;       // Digital output to 24V solenoid driver
const int PIN_VALVE_AO = A0;      // Analog out (0-10V path); replace with actual AO channel
const int PIN_ESTOP_IN = 22;      // Digital input (optional)

// ADS1234 interface (SPI or parallel; adapt as needed)
// TODO: add your ADS1234 driver include and pins

// ==== Register Addresses (mirror of the map) ====
// Coils
enum {
  COIL_CMD_START=0, COIL_CMD_STOP, COIL_CMD_RESET_FAULTS, COIL_CMD_ABORT,
  COIL_CMD_MODE_IDLE, COIL_CMD_MODE_FILL, COIL_CMD_MODE_BALANCE, COIL_CMD_MODE_TEST,
  COIL_CMD_MODE_SHUTDOWN, COIL_CMD_MANUAL_SOL, COIL_CMD_TARE, COIL_CMD_ENABLE_REMOTE
};

// Discrete inputs
enum {
  DI_ESTOP_OK=0, DI_FAULT_ACTIVE, DI_REMOTE_ENABLED,
  DI_MODE_IDLE, DI_MODE_FILL, DI_MODE_BALANCE, DI_MODE_TEST, DI_MODE_SHUTDOWN,
  DI_SOLENOID_ON, DI_VALVE_ENABLED, DI_SCALE_OK, DI_HEARTBEAT_OK
};

// Holding registers
const uint16_t HR_SP_TARGET_MASS = 0;       // g x10
const uint16_t HR_SP_FILL_TIMEOUT = 1;      // s
const uint16_t HR_SP_FLOW_TARGET = 2;       // g/s x10
const uint16_t HR_SP_KP = 3;                // x100
const uint16_t HR_SP_KI = 4;                // x100
const uint16_t HR_SP_KD = 5;                // x100
const uint16_t HR_SP_VALVE_MIN = 6;         // mV
const uint16_t HR_SP_VALVE_MAX = 7;         // mV
const uint16_t HR_SP_BALANCE_S = 8;         // s
const uint16_t HR_SP_TEST_S = 9;            // s
const uint16_t HR_SP_DERIV_MS = 10;         // ms
const uint16_t HR_SP_HYST = 11;             // g/s x10
const uint16_t HR_SP_LOG_MS = 12;           // ms (advisory)
const uint16_t HR_CMD_SEQ = 500;            // heartbeat

// Input registers
const uint16_t IR_MASS_LO = 0;              // g x10, 32-bit
const uint16_t IR_MASS_HI = 1;
const uint16_t IR_FLOW_LO = 2;              // g/s x10, 32-bit signed
const uint16_t IR_FLOW_HI = 3;
const uint16_t IR_VALVE_MV = 4;             // mV
const uint16_t IR_STATE_CODE = 5;
const uint16_t IR_LAST_FAULT = 6;
const uint16_t IR_SCALE_RAW_LO = 7;
const uint16_t IR_SCALE_RAW_HI = 8;
const uint16_t IR_LOOP_OUT_PCT = 9;         // % x10
const uint16_t IR_ERR_LO = 10;              // g/s x10, 32-bit signed
const uint16_t IR_ERR_HI = 11;
const uint16_t IR_TEMP_C = 12;              // optional
const uint16_t IR_HB_ECHO = 20;

// ==== State machine ====
enum Mode {IDLE=0, FILL=1, BALANCE_TEST=2, TEST=3, SHUTDOWN=4, FAULT=100};
Mode mode = IDLE;
uint16_t lastFault = 0;

// ==== Runtime variables ====
volatile int32_t mass_g_x10 = 0;    // from ADS1234
volatile int32_t flow_gps_x10 = 0;  // computed dM/dt
float pid_i = 0.f;
uint32_t lastDerivMs = 0;
uint32_t lastUpdate = 0;

// Helpers
static inline void writeI32(uint16_t loAddr, int32_t v){
  ModbusTCPServer.inputRegisterWrite(loAddr, (uint16_t)(v & 0xFFFF));
  ModbusTCPServer.inputRegisterWrite(loAddr+1, (uint16_t)((uint32_t)v >> 16));
}
static inline int32_t readI32(uint16_t loAddr){
  uint32_t lo = ModbusTCPServer.inputRegisterRead(loAddr);
  uint32_t hi = ModbusTCPServer.inputRegisterRead(loAddr+1);
  return (int32_t)((hi<<16) | lo);
}

// Clamp utility
static inline int clampi(int v, int lo, int hi){ return v<lo?lo:(v>hi?hi:v); }

// ====== Hardware stubs (replace with real drivers) ======
bool readEstopOk(){ pinMode(PIN_ESTOP_IN, INPUT_PULLUP); return digitalRead(PIN_ESTOP_IN)==HIGH; }
int32_t readScaleCounts(){ /* TODO: ADS1234 read */ return 0; }
int32_t countsToMass_g_x10(int32_t counts){ /* TODO: calibration */ return counts; }
void setSolenoid(bool on){ digitalWrite(PIN_SOLENOID, on?HIGH:LOW); }
void setValveMilliVolts(int mv){ mv = clampi(mv,0,10000); // map 0..10000 mV to 0..4095 DAC counts, for example
  int dac = map(mv,0,10000,0,4095); analogWrite(PIN_VALVE_AO, dac);
}

void fault(uint16_t code){ lastFault = code; mode = FAULT; ModbusTCPServer.discreteInputWrite(DI_FAULT_ACTIVE, true); setSolenoid(false); setValveMilliVolts(0); }

void setup(){
  pinMode(PIN_SOLENOID, OUTPUT); setSolenoid(false);
  // AO init as needed for 0-10V path

  Ethernet.begin(mac, ip);
  if(!ModbusTCPServer.begin()) while(1);

  // Size blocks
  ModbusTCPServer.configureCoils(0, 64);
  ModbusTCPServer.configureDiscreteInputs(0, 64);
  ModbusTCPServer.configureHoldingRegisters(0, 600);
  ModbusTCPServer.configureInputRegisters(0, 600);

  // Defaults
  ModbusTCPServer.holdingRegisterWrite(HR_SP_TARGET_MASS, 10000); // 1000.0 g
  ModbusTCPServer.holdingRegisterWrite(HR_SP_FILL_TIMEOUT, 120);
  ModbusTCPServer.holdingRegisterWrite(HR_SP_FLOW_TARGET, 200);   // 20.0 g/s
  ModbusTCPServer.holdingRegisterWrite(HR_SP_KP, 150);            // 1.50
  ModbusTCPServer.holdingRegisterWrite(HR_SP_KI, 20);             // 0.20
  ModbusTCPServer.holdingRegisterWrite(HR_SP_KD, 0);
  ModbusTCPServer.holdingRegisterWrite(HR_SP_VALVE_MIN, 500);     // 0.5 V
  ModbusTCPServer.holdingRegisterWrite(HR_SP_VALVE_MAX, 9500);    // 9.5 V
  ModbusTCPServer.holdingRegisterWrite(HR_SP_BALANCE_S, 10);
  ModbusTCPServer.holdingRegisterWrite(HR_SP_TEST_S, 30);
  ModbusTCPServer.holdingRegisterWrite(HR_SP_DERIV_MS, 500);
  ModbusTCPServer.holdingRegisterWrite(HR_SP_HYST, 5);            // 0.5 g/s

  ModbusTCPServer.discreteInputWrite(DI_MODE_IDLE, true);
}

// Heartbeat window
uint32_t lastHbMs = 0;
uint16_t lastSeq = 0;
bool remoteEnabled = false;

void loop(){
  ModbusTCPServer.poll();
  const uint32_t now = millis();

  // ==== Heartbeat echo & remote enable timeout ====
  uint16_t seq = ModbusTCPServer.holdingRegisterRead(HR_CMD_SEQ);
  if(seq != lastSeq){ lastSeq = seq; lastHbMs = now; ModbusTCPServer.inputRegisterWrite(IR_HB_ECHO, seq); }
  bool hbOk = (now - lastHbMs) < 5000; // 5s window
  ModbusTCPServer.discreteInputWrite(DI_HEARTBEAT_OK, hbOk);

  if(ModbusTCPServer.coilRead(COIL_CMD_ENABLE_REMOTE)){
    ModbusTCPServer.discreteInputWrite(DI_REMOTE_ENABLED, true);
    remoteEnabled = true;
    // auto-clear the request
    ModbusTCPServer.coilWrite(COIL_CMD_ENABLE_REMOTE, 0);
    lastHbMs = now; // start window
  }
  if(remoteEnabled && (now - lastHbMs) > (5*60*1000UL)){
    remoteEnabled = false;
    ModbusTCPServer.discreteInputWrite(DI_REMOTE_ENABLED, false);
  }

  // ==== Read scale, compute mass & flow ====
  int32_t raw = readScaleCounts();
  int32_t mass = countsToMass_g_x10(raw);
  uint32_t dms = now - lastDerivMs; if(dms<1) dms=1; lastDerivMs = now;
  static int32_t lastMass = 0;
  int32_t dm = mass - lastMass; lastMass = mass;
  int32_t flow_gps10 = (int32_t)((dm * 1000L) / (int32_t)dms); // g/s x10

  mass_g_x10 = mass; flow_gps_x10 = flow_gps10;

  // Publish telemetry
  writeI32(IR_MASS_LO, mass_g_x10);
  writeI32(IR_FLOW_LO, flow_gps_x10);
  ModbusTCPServer.inputRegisterWrite(IR_VALVE_MV, 0); // updated later
  ModbusTCPServer.inputRegisterWrite(IR_STATE_CODE, (uint16_t)mode);
  ModbusTCPServer.inputRegisterWrite(IR_LAST_FAULT, lastFault);

  // Scale OK heuristic
  bool scaleOk = true; // TODO add real validation
  ModbusTCPServer.discreteInputWrite(DI_SCALE_OK, scaleOk);

  // ==== Safety checks ====
  bool estopOk = readEstopOk();
  ModbusTCPServer.discreteInputWrite(DI_ESTOP_OK, estopOk);
  if(!estopOk){ fault(101); }

  // ==== Command edges ====
  auto takeEdge = [](int coil){ bool s=ModbusTCPServer.coilRead(coil); if(s){ ModbusTCPServer.coilWrite(coil,0); return true;} return false; };
  if(takeEdge(COIL_CMD_TARE)){ /* TODO: run tare */ }
  if(takeEdge(COIL_CMD_RESET_FAULTS)){ if(mode==FAULT){ lastFault=0; ModbusTCPServer.discreteInputWrite(DI_FAULT_ACTIVE,false); mode=IDLE; }}
  if(takeEdge(COIL_CMD_ABORT)){ mode=SHUTDOWN; }

  // Mode requests
  if(takeEdge(COIL_CMD_MODE_IDLE)) mode=IDLE;
  if(takeEdge(COIL_CMD_MODE_FILL)) mode=FILL;
  if(takeEdge(COIL_CMD_MODE_BALANCE)) mode=BALANCE_TEST;
  if(takeEdge(COIL_CMD_MODE_TEST)) mode=TEST;
  if(takeEdge(COIL_CMD_MODE_SHUTDOWN)) mode=SHUTDOWN;

  // Start/Stop only honored when remote enabled and hb ok
  if(remoteEnabled && hbOk){
    if(takeEdge(COIL_CMD_START)){
      if(mode==IDLE) mode = FILL;
    }
    if(takeEdge(COIL_CMD_STOP)){
      mode = SHUTDOWN;
    }
  }

  // ==== State machine ====
  static uint32_t stateTs = now;
  auto enter = [&](Mode m){ mode=m; stateTs=now; };

  // Clear mode DI flags
  ModbusTCPServer.discreteInputWrite(DI_MODE_IDLE,false);
  ModbusTCPServer.discreteInputWrite(DI_MODE_FILL,false);
  ModbusTCPServer.discreteInputWrite(DI_MODE_BALANCE,false);
  ModbusTCPServer.discreteInputWrite(DI_MODE_TEST,false);
  ModbusTCPServer.discreteInputWrite(DI_MODE_SHUTDOWN,false);

  int valve_mV = 0;
  bool sol_on = false;

  switch(mode){
    case IDLE:
      ModbusTCPServer.discreteInputWrite(DI_MODE_IDLE,true);
      setSolenoid(false); setValveMilliVolts(0);
      pid_i = 0.f;
      break;

    case FILL: {
      ModbusTCPServer.discreteInputWrite(DI_MODE_FILL,true);
      sol_on = true; setSolenoid(true);
      // Transition when mass >= target OR timeout
      uint16_t target = ModbusTCPServer.holdingRegisterRead(HR_SP_TARGET_MASS);
      uint16_t timeout_s = ModbusTCPServer.holdingRegisterRead(HR_SP_FILL_TIMEOUT);
      if((uint32_t)(now - stateTs) > (uint32_t)timeout_s*1000UL){ fault(103); break; }
      if(mass_g_x10 >= (int32_t)target){ setSolenoid(false); enter(BALANCE_TEST); }
      break; }

    case BALANCE_TEST: {
      ModbusTCPServer.discreteInputWrite(DI_MODE_BALANCE,true);
      // Flow PID to stabilize
      int sp = (int16_t)ModbusTCPServer.holdingRegisterRead(HR_SP_FLOW_TARGET); // g/s x10
      int err = sp - flow_gps_x10;
      // PID params
      float kp = ModbusTCPServer.holdingRegisterRead(HR_SP_KP)/100.0f;
      float ki = ModbusTCPServer.holdingRegisterRead(HR_SP_KI)/100.0f;
      float kd = ModbusTCPServer.holdingRegisterRead(HR_SP_KD)/100.0f;
      static int lastErr = 0;
      float dt = (now - lastUpdate)/1000.0f; if(dt<=0) dt = 0.001f;
      pid_i += err * ki * dt;
      float d = (err - lastErr) * kd / dt;
      float u = err * kp + pid_i + d; // controller output in "g/s x10 equivalent"
      lastErr = err; lastUpdate = now;

      // Map controller output to mV; here we just scale (tune as needed)
      int mVmin = ModbusTCPServer.holdingRegisterRead(HR_SP_VALVE_MIN);
      int mVmax = ModbusTCPServer.holdingRegisterRead(HR_SP_VALVE_MAX);
      // naive mapping: 1 unit -> 1 mV; adjust scaling factor to your plant
      valve_mV = clampi((int)(u) , mVmin, mVmax);
      setValveMilliVolts(valve_mV);
      ModbusTCPServer.discreteInputWrite(DI_VALVE_ENABLED, true);

      // Transition to TEST after balance duration
      uint16_t bal_s = ModbusTCPServer.holdingRegisterRead(HR_SP_BALANCE_S);
      if((uint32_t)(now - stateTs) > (uint32_t)bal_s*1000UL) enter(TEST);

      // publish loop terms
      ModbusTCPServer.inputRegisterWrite(IR_LOOP_OUT_PCT, (uint16_t)map(valve_mV, mVmin, mVmax, 0, 1000));
      writeI32(IR_ERR_LO, (int32_t)err);
      break; }

    case TEST: {
      ModbusTCPServer.discreteInputWrite(DI_MODE_TEST,true);
      // Continue PID as in BALANCE_TEST
      // (duplicate of above for brevity)
      int sp = (int16_t)ModbusTCPServer.holdingRegisterRead(HR_SP_FLOW_TARGET);
      int err = sp - flow_gps_x10;
      float kp = ModbusTCPServer.holdingRegisterRead(HR_SP_KP)/100.0f;
      float ki = ModbusTCPServer.holdingRegisterRead(HR_SP_KI)/100.0f;
      float kd = ModbusTCPServer.holdingRegisterRead(HR_SP_KD)/100.0f;
      static int lastErr2 = 0;
      float dt = (now - lastUpdate)/1000.0f; if(dt<=0) dt = 0.001f;
      pid_i += err * ki * dt;
      float d = (err - lastErr2) * kd / dt;
      float u = err * kp + pid_i + d;
      lastErr2 = err; lastUpdate = now;
      int mVmin = ModbusTCPServer.holdingRegisterRead(HR_SP_VALVE_MIN);
      int mVmax = ModbusTCPServer.holdingRegisterRead(HR_SP_VALVE_MAX);
      valve_mV = clampi((int)(u), mVmin, mVmax);
      setValveMilliVolts(valve_mV);
      ModbusTCPServer.discreteInputWrite(DI_VALVE_ENABLED, true);

      // End TEST after duration, then SHUTDOWN
      uint16_t test_s = ModbusTCPServer.holdingRegisterRead(HR_SP_TEST_S);
      if((uint32_t)(now - stateTs) > (uint32_t)test_s*1000UL){ enter(SHUTDOWN); }
      break; }

    case SHUTDOWN:
      ModbusTCPServer.discreteInputWrite(DI_MODE_SHUTDOWN,true);
      setSolenoid(false); setValveMilliVolts(0);
      // After safe, go back to IDLE
      if(remoteEnabled && takeEdge(COIL_CMD_RESET_FAULTS)) { mode=IDLE; }
      break;

    case FAULT:
      setSolenoid(false); setValveMilliVolts(0);
      // wait for RESET_FAULTS
      break;
  }

  // Write back actuator/loop values
  ModbusTCPServer.discreteInputWrite(DI_SOLENOID_ON, sol_on);
  ModbusTCPServer.inputRegisterWrite(IR_VALVE_MV, (uint16_t)valve_mV);
}

