#ifndef GAUGE_REED_CONTROL_H
#define GAUGE_REED_CONTROL_H

#include <Arduino.h>

// ------------------------------------------------------------
// Configuration constants
// ------------------------------------------------------------

static const uint8_t NUM_REEDS  = 8;
static const uint8_t NUM_GAUGES = 4;

static const unsigned long REED_DEBOUNCE_US = 5000;   // 5 ms

// ------------------------------------------------------------
// Enums
// ------------------------------------------------------------

enum GaugeMode {
  GAUGE_DISABLED = 0,
  GAUGE_SINGLE_A = 1,
  GAUGE_DUAL     = 2
};

enum MachineState {
  MS_IDLE = 0,
  MS_FILL,
  MS_BALANCE_TEST,
  MS_TEST,
  MS_SHUTDOWN,
  MS_FAULT
};

enum GaugeFaultBits {
  GF_NONE            = 0,
  GF_REED_A_MISSING  = 1 << 0,
  GF_REED_B_MISSING  = 1 << 1,
  GF_REED_MISMATCH   = 1 << 2,
  GF_DISABLED        = 1 << 3
};

// ------------------------------------------------------------
// Reed input struct
// ------------------------------------------------------------

struct ReedInput {
  int pin;
  bool enabled;

  bool stableState;
  bool lastStableState;

  volatile bool edgePending;
  volatile bool rawState;
  volatile unsigned long edgeTimeUs;

  unsigned long lastProcessedEdgeUs;

  unsigned long closureCount;
  unsigned long closeStartUs;
  unsigned long openTimeUs;
  unsigned long lastClosedTimeUs;
  unsigned long maxClosedTimeUs;
  unsigned long minClosedTimeUs;

  bool newClosureEvent;
  bool newOpenEvent;
};

// ------------------------------------------------------------
// Gauge struct
// ------------------------------------------------------------

struct GaugeChannel {
  bool enabled;
  GaugeMode mode;

  unsigned long lastEventTimeUs;
  long lastSkewUs;
  unsigned long lastClosedTimeAUs;
  unsigned long lastClosedTimeBUs;
  unsigned long eventCount;
  bool newEvent;

  unsigned long lastProcessedCountA;
  unsigned long lastProcessedCountB;

  bool reedAClosed;
  bool reedBClosed;

  bool fault;
  uint32_t faultBits;
  MachineState machineState;

  float weight;
  float valveSetpoint;
  bool solenoidOpen;
};

// ------------------------------------------------------------
// Globals
// ------------------------------------------------------------

extern ReedInput reed[NUM_REEDS];
extern GaugeChannel gauge[NUM_GAUGES];

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------

void initGaugeReedControl();
void updateGaugeReedControl();

// Fixed gauge/reed mapping helpers
int gaugeReedAIndex(uint8_t gaugeIndex);
int gaugeReedBIndex(uint8_t gaugeIndex);

// Mode helpers
const char* gaugeModeToString(GaugeMode mode);
const char* machineStateToString(MachineState state);

// Configuration API
bool setGaugeMode(uint8_t gaugeNumber1to4, GaugeMode mode);
bool setGaugeModeFromInt(uint8_t gaugeNumber1to4, int modeValue);
GaugeMode getGaugeMode(uint8_t gaugeNumber1to4);
void resetGaugeStats(uint8_t gaugeIndex0to3);
void resetAllGaugeStats();
void resetReedStats(uint8_t reedIndex0to7);
void resetAllReedStats();
void resetGaugeFaults(uint8_t gaugeIndex0to3);
void resetAllGaugeFaults();

// Status strings
String getReedStatusString(uint8_t reedIndex0to7);
String getGaugeStatusString(uint8_t gaugeIndex0to3);
String getAllGaugeStatusString();
String getAllGaugeConfigString();

// ISR wrappers
void reedISR0();
void reedISR1();
void reedISR2();
void reedISR3();
void reedISR4();
void reedISR5();
void reedISR6();
void reedISR7();

#endif