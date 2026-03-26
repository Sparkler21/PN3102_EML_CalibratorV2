#include "GaugeReedControl.h"

// ------------------------------------------------------------
// Globals
// ------------------------------------------------------------

ReedInput reed[NUM_REEDS];
GaugeChannel gauge[NUM_GAUGES];

// ------------------------------------------------------------
// Internal forward declarations
// ------------------------------------------------------------

static void configureReedPins();
static void initReedInputs();
static void initGaugeChannels();
static void handleReedInterrupt(uint8_t index);
static void processReedInput(uint8_t i);
static void updateAllReedInputs();
static void updateGaugeChannels();

// ------------------------------------------------------------
// Fixed mapping helpers
// ------------------------------------------------------------

int gaugeReedAIndex(uint8_t gaugeIndex) {
  if (gaugeIndex >= NUM_GAUGES) return -1;
  return gaugeIndex * 2;
}

int gaugeReedBIndex(uint8_t gaugeIndex) {
  if (gaugeIndex >= NUM_GAUGES) return -1;
  return gaugeIndex * 2 + 1;
}

// ------------------------------------------------------------
// String helpers
// ------------------------------------------------------------

const char* gaugeModeToString(GaugeMode mode) {
  switch (mode) {
    case GAUGE_DISABLED: return "DISABLED";
    case GAUGE_SINGLE_A: return "SINGLE_A";
    case GAUGE_DUAL:     return "DUAL";
    default:             return "UNKNOWN";
  }
}

const char* machineStateToString(MachineState state) {
  switch (state) {
    case MS_IDLE:         return "IDLE";
    case MS_FILL:         return "FILL";
    case MS_BALANCE_TEST: return "BALANCE_TEST";
    case MS_TEST:         return "TEST";
    case MS_SHUTDOWN:     return "SHUTDOWN";
    case MS_FAULT:        return "FAULT";
    default:              return "UNKNOWN";
  }
}

// ------------------------------------------------------------
// Configuration
// Edit reed pin assignments here only
// ------------------------------------------------------------

static void configureReedPins() {
  // Fixed reed channels 0..7
  // Change these pin names to match your Portenta wiring.

  reed[0].pin = D0; reed[0].enabled = true;
  reed[1].pin = D1; reed[1].enabled = true;
  reed[2].pin = D2; reed[2].enabled = true;
  reed[3].pin = D3; reed[3].enabled = true;
  reed[4].pin = D4; reed[4].enabled = true;
  reed[5].pin = D5; reed[5].enabled = true;
  reed[6].pin = D6; reed[6].enabled = true;
  reed[7].pin = D7; reed[7].enabled = true;
}

// ------------------------------------------------------------
// Public init / update
// ------------------------------------------------------------

void initGaugeReedControl() {
  configureReedPins();
  initReedInputs();
  initGaugeChannels();
}

void updateGaugeReedControl() {
  updateAllReedInputs();
  updateGaugeChannels();
}

// ------------------------------------------------------------
// Public config API
// ------------------------------------------------------------

bool setGaugeMode(uint8_t gaugeNumber1to4, GaugeMode mode) {
  if (gaugeNumber1to4 < 1 || gaugeNumber1to4 > NUM_GAUGES) {
    return false;
  }

  if (mode != GAUGE_DISABLED &&
      mode != GAUGE_SINGLE_A &&
      mode != GAUGE_DUAL) {
    return false;
  }

  uint8_t g = gaugeNumber1to4 - 1;

  gauge[g].mode = mode;
  gauge[g].enabled = (mode != GAUGE_DISABLED);

  resetGaugeStats(g);
  resetGaugeFaults(g);

  if (mode == GAUGE_DISABLED) {
    gauge[g].faultBits |= GF_DISABLED;
  }

  return true;
}

bool setGaugeModeFromInt(uint8_t gaugeNumber1to4, int modeValue) {
  switch (modeValue) {
    case 0: return setGaugeMode(gaugeNumber1to4, GAUGE_DISABLED);
    case 1: return setGaugeMode(gaugeNumber1to4, GAUGE_SINGLE_A);
    case 2: return setGaugeMode(gaugeNumber1to4, GAUGE_DUAL);
    default: return false;
  }
}

GaugeMode getGaugeMode(uint8_t gaugeNumber1to4) {
  if (gaugeNumber1to4 < 1 || gaugeNumber1to4 > NUM_GAUGES) {
    return GAUGE_DISABLED;
  }
  return gauge[gaugeNumber1to4 - 1].mode;
}

// ------------------------------------------------------------
// Reset helpers
// ------------------------------------------------------------

void resetGaugeStats(uint8_t gaugeIndex0to3) {
  if (gaugeIndex0to3 >= NUM_GAUGES) return;

  gauge[gaugeIndex0to3].lastEventTimeUs = 0;
  gauge[gaugeIndex0to3].lastSkewUs = 0;
  gauge[gaugeIndex0to3].lastClosedTimeAUs = 0;
  gauge[gaugeIndex0to3].lastClosedTimeBUs = 0;
  gauge[gaugeIndex0to3].eventCount = 0;
  gauge[gaugeIndex0to3].newEvent = false;
  gauge[gaugeIndex0to3].lastProcessedCountA = 0;
  gauge[gaugeIndex0to3].lastProcessedCountB = 0;
}

void resetAllGaugeStats() {
  for (uint8_t g = 0; g < NUM_GAUGES; g++) {
    resetGaugeStats(g);
  }
}

void resetReedStats(uint8_t reedIndex0to7) {
  if (reedIndex0to7 >= NUM_REEDS) return;

  reed[reedIndex0to7].closureCount = 0;
  reed[reedIndex0to7].closeStartUs = 0;
  reed[reedIndex0to7].openTimeUs = 0;
  reed[reedIndex0to7].lastClosedTimeUs = 0;
  reed[reedIndex0to7].maxClosedTimeUs = 0;
  reed[reedIndex0to7].minClosedTimeUs = 0xFFFFFFFFUL;
  reed[reedIndex0to7].newClosureEvent = false;
  reed[reedIndex0to7].newOpenEvent = false;
}

void resetAllReedStats() {
  for (uint8_t i = 0; i < NUM_REEDS; i++) {
    resetReedStats(i);
  }
}

void resetGaugeFaults(uint8_t gaugeIndex0to3) {
  if (gaugeIndex0to3 >= NUM_GAUGES) return;

  gauge[gaugeIndex0to3].fault = false;
  gauge[gaugeIndex0to3].faultBits = GF_NONE;

  if (!gauge[gaugeIndex0to3].enabled || gauge[gaugeIndex0to3].mode == GAUGE_DISABLED) {
    gauge[gaugeIndex0to3].faultBits |= GF_DISABLED;
  }
}

void resetAllGaugeFaults() {
  for (uint8_t g = 0; g < NUM_GAUGES; g++) {
    resetGaugeFaults(g);
  }
}

// ------------------------------------------------------------
// Init
// ------------------------------------------------------------

static void initReedInputs() {
  for (uint8_t i = 0; i < NUM_REEDS; i++) {
    reed[i].stableState = HIGH;
    reed[i].lastStableState = HIGH;

    reed[i].edgePending = false;
    reed[i].rawState = HIGH;
    reed[i].edgeTimeUs = 0;

    reed[i].lastProcessedEdgeUs = 0;

    reed[i].closureCount = 0;
    reed[i].closeStartUs = 0;
    reed[i].openTimeUs = 0;
    reed[i].lastClosedTimeUs = 0;
    reed[i].maxClosedTimeUs = 0;
    reed[i].minClosedTimeUs = 0xFFFFFFFFUL;

    reed[i].newClosureEvent = false;
    reed[i].newOpenEvent = false;

    if (!reed[i].enabled || reed[i].pin < 0) {
      continue;
    }

    pinMode(reed[i].pin, INPUT_PULLUP);

    bool initialState = digitalRead(reed[i].pin);
    reed[i].stableState = initialState;
    reed[i].lastStableState = initialState;
    reed[i].rawState = initialState;
  }

  if (reed[0].enabled && reed[0].pin >= 0) attachInterrupt(digitalPinToInterrupt(reed[0].pin), reedISR0, CHANGE);
  if (reed[1].enabled && reed[1].pin >= 0) attachInterrupt(digitalPinToInterrupt(reed[1].pin), reedISR1, CHANGE);
  if (reed[2].enabled && reed[2].pin >= 0) attachInterrupt(digitalPinToInterrupt(reed[2].pin), reedISR2, CHANGE);
  if (reed[3].enabled && reed[3].pin >= 0) attachInterrupt(digitalPinToInterrupt(reed[3].pin), reedISR3, CHANGE);
  if (reed[4].enabled && reed[4].pin >= 0) attachInterrupt(digitalPinToInterrupt(reed[4].pin), reedISR4, CHANGE);
  if (reed[5].enabled && reed[5].pin >= 0) attachInterrupt(digitalPinToInterrupt(reed[5].pin), reedISR5, CHANGE);
  if (reed[6].enabled && reed[6].pin >= 0) attachInterrupt(digitalPinToInterrupt(reed[6].pin), reedISR6, CHANGE);
  if (reed[7].enabled && reed[7].pin >= 0) attachInterrupt(digitalPinToInterrupt(reed[7].pin), reedISR7, CHANGE);
}

static void initGaugeChannels() {
  for (uint8_t g = 0; g < NUM_GAUGES; g++) {
    gauge[g].enabled = false;
    gauge[g].mode = GAUGE_DISABLED;

    gauge[g].lastEventTimeUs = 0;
    gauge[g].lastSkewUs = 0;
    gauge[g].lastClosedTimeAUs = 0;
    gauge[g].lastClosedTimeBUs = 0;
    gauge[g].eventCount = 0;
    gauge[g].newEvent = false;

    gauge[g].lastProcessedCountA = 0;
    gauge[g].lastProcessedCountB = 0;

    gauge[g].reedAClosed = false;
    gauge[g].reedBClosed = false;

    gauge[g].fault = false;
    gauge[g].faultBits = GF_DISABLED;
    gauge[g].machineState = MS_IDLE;

    gauge[g].weight = 0.0f;
    gauge[g].valveSetpoint = 0.0f;
    gauge[g].solenoidOpen = false;
  }
}

// ------------------------------------------------------------
// ISR handling
// ------------------------------------------------------------

static void handleReedInterrupt(uint8_t index) {
  if (index >= NUM_REEDS) return;
  if (!reed[index].enabled || reed[index].pin < 0) return;

  unsigned long nowUs = micros();
  reed[index].rawState = digitalRead(reed[index].pin);
  reed[index].edgeTimeUs = nowUs;
  reed[index].edgePending = true;
}

void reedISR0() { handleReedInterrupt(0); }
void reedISR1() { handleReedInterrupt(1); }
void reedISR2() { handleReedInterrupt(2); }
void reedISR3() { handleReedInterrupt(3); }
void reedISR4() { handleReedInterrupt(4); }
void reedISR5() { handleReedInterrupt(5); }
void reedISR6() { handleReedInterrupt(6); }
void reedISR7() { handleReedInterrupt(7); }

// ------------------------------------------------------------
// Reed processing
// ------------------------------------------------------------

static void processReedInput(uint8_t i) {
  if (i >= NUM_REEDS) return;
  if (!reed[i].enabled || reed[i].pin < 0) return;

  bool pending;
  bool raw;
  unsigned long edgeUs;

  noInterrupts();
  pending = reed[i].edgePending;
  raw = reed[i].rawState;
  edgeUs = reed[i].edgeTimeUs;
  interrupts();

  if (!pending) return;

  unsigned long nowUs = micros();

  if ((nowUs - edgeUs) < REED_DEBOUNCE_US) {
    return;
  }

  bool confirmedState = digitalRead(reed[i].pin);

  if (confirmedState != raw) {
    return;
  }

  noInterrupts();
  reed[i].edgePending = false;
  interrupts();

  if (confirmedState == reed[i].stableState) {
    return;
  }

  reed[i].lastStableState = reed[i].stableState;
  reed[i].stableState = confirmedState;
  reed[i].lastProcessedEdgeUs = edgeUs;

  if (reed[i].lastStableState == HIGH && reed[i].stableState == LOW) {
    reed[i].closureCount++;
    reed[i].closeStartUs = edgeUs;
    reed[i].newClosureEvent = true;
  }

  if (reed[i].lastStableState == LOW && reed[i].stableState == HIGH) {
    reed[i].openTimeUs = edgeUs;
    reed[i].newOpenEvent = true;

    if (reed[i].closeStartUs > 0 && reed[i].openTimeUs >= reed[i].closeStartUs) {
      unsigned long durationUs = reed[i].openTimeUs - reed[i].closeStartUs;
      reed[i].lastClosedTimeUs = durationUs;

      if (durationUs > reed[i].maxClosedTimeUs) {
        reed[i].maxClosedTimeUs = durationUs;
      }

      if (durationUs < reed[i].minClosedTimeUs) {
        reed[i].minClosedTimeUs = durationUs;
      }
    }
  }
}

static void updateAllReedInputs() {
  for (uint8_t i = 0; i < NUM_REEDS; i++) {
    processReedInput(i);
  }
}

// ------------------------------------------------------------
// Gauge processing
// ------------------------------------------------------------

static void updateGaugeChannels() {
  for (uint8_t g = 0; g < NUM_GAUGES; g++) {
    gauge[g].newEvent = false;
    gauge[g].fault = false;
    gauge[g].faultBits &= ~(GF_REED_A_MISSING | GF_REED_B_MISSING | GF_REED_MISMATCH);

    int a = gaugeReedAIndex(g);
    int b = gaugeReedBIndex(g);

    bool hasA = (a >= 0 && a < NUM_REEDS && reed[a].enabled && reed[a].pin >= 0);
    bool hasB = (b >= 0 && b < NUM_REEDS && reed[b].enabled && reed[b].pin >= 0);

    if (!gauge[g].enabled || gauge[g].mode == GAUGE_DISABLED) {
      gauge[g].reedAClosed = false;
      gauge[g].reedBClosed = false;
      gauge[g].faultBits |= GF_DISABLED;
      continue;
    } else {
      gauge[g].faultBits &= ~GF_DISABLED;
    }

    if (!hasA) {
      gauge[g].fault = true;
      gauge[g].faultBits |= GF_REED_A_MISSING;
      gauge[g].reedAClosed = false;
      gauge[g].reedBClosed = false;
      continue;
    }

    gauge[g].reedAClosed = (reed[a].stableState == LOW);
    gauge[g].reedBClosed = hasB ? (reed[b].stableState == LOW) : false;

    unsigned long countA = reed[a].closureCount;
    bool newA = (countA != gauge[g].lastProcessedCountA);

    if (gauge[g].mode == GAUGE_SINGLE_A) {
      if (newA) {
        gauge[g].lastProcessedCountA = countA;
        gauge[g].lastEventTimeUs = reed[a].closeStartUs;
        gauge[g].lastSkewUs = 0;
        gauge[g].lastClosedTimeAUs = reed[a].lastClosedTimeUs;
        gauge[g].lastClosedTimeBUs = 0;
        gauge[g].eventCount++;
        gauge[g].newEvent = true;
      }
      continue;
    }

    if (gauge[g].mode == GAUGE_DUAL) {
      if (!hasB) {
        gauge[g].fault = true;
        gauge[g].faultBits |= GF_REED_B_MISSING;
        continue;
      }

      unsigned long countB = reed[b].closureCount;
      bool newB = (countB != gauge[g].lastProcessedCountB);

      if (countA != countB) {
        gauge[g].faultBits |= GF_REED_MISMATCH;
      }

      if (newA && newB) {
        unsigned long startA = reed[a].closeStartUs;
        unsigned long startB = reed[b].closeStartUs;

        gauge[g].lastProcessedCountA = countA;
        gauge[g].lastProcessedCountB = countB;

        gauge[g].lastEventTimeUs = (startA > startB) ? startA : startB;
        gauge[g].lastSkewUs = (long)startA - (long)startB;
        gauge[g].lastClosedTimeAUs = reed[a].lastClosedTimeUs;
        gauge[g].lastClosedTimeBUs = reed[b].lastClosedTimeUs;
        gauge[g].eventCount++;
        gauge[g].newEvent = true;
      }
    }
  }
}

// ------------------------------------------------------------
// Status strings
// ------------------------------------------------------------

String getReedStatusString(uint8_t reedIndex0to7) {
  if (reedIndex0to7 >= NUM_REEDS) {
    return "ERROR,BAD_REED";
  }

  String s = "REED_STATUS,";
  s += String(reedIndex0to7 + 1) + ",";
  s += String(reed[reedIndex0to7].enabled ? 1 : 0) + ",";
  s += String(reed[reedIndex0to7].pin) + ",";
  s += String(reed[reedIndex0to7].stableState == LOW ? 1 : 0) + ",";
  s += String(reed[reedIndex0to7].closureCount) + ",";
  s += String(reed[reedIndex0to7].lastClosedTimeUs / 1000.0f, 3) + ",";
  s += String(reed[reedIndex0to7].maxClosedTimeUs / 1000.0f, 3) + ",";
  if (reed[reedIndex0to7].minClosedTimeUs == 0xFFFFFFFFUL) {
    s += "0.000";
  } else {
    s += String(reed[reedIndex0to7].minClosedTimeUs / 1000.0f, 3);
  }
  return s;
}

String getGaugeStatusString(uint8_t gaugeIndex0to3) {
  if (gaugeIndex0to3 >= NUM_GAUGES) {
    return "ERROR,BAD_GAUGE";
  }

  int a = gaugeReedAIndex(gaugeIndex0to3);
  int b = gaugeReedBIndex(gaugeIndex0to3);

  String s = "GAUGE_STATUS,";
  s += String(gaugeIndex0to3 + 1) + ",";
  s += String(gauge[gaugeIndex0to3].enabled ? 1 : 0) + ",";
  s += String(gaugeModeToString(gauge[gaugeIndex0to3].mode)) + ",";
  s += String(machineStateToString(gauge[gaugeIndex0to3].machineState)) + ",";
  s += String(a) + ",";
  s += String(b) + ",";
  s += String(gauge[gaugeIndex0to3].reedAClosed ? 1 : 0) + ",";
  s += String(gauge[gaugeIndex0to3].reedBClosed ? 1 : 0) + ",";
  s += String(gauge[gaugeIndex0to3].eventCount) + ",";
  s += String(gauge[gaugeIndex0to3].lastSkewUs) + ",";
  s += String(gauge[gaugeIndex0to3].lastClosedTimeAUs / 1000.0f, 3) + ",";
  s += String(gauge[gaugeIndex0to3].lastClosedTimeBUs / 1000.0f, 3) + ",";
  s += String(gauge[gaugeIndex0to3].faultBits);
  return s;
}

String getAllGaugeStatusString() {
  String s = "GAUGES";
  for (uint8_t g = 0; g < NUM_GAUGES; g++) {
    s += ",";
    s += String(g + 1);
    s += ":";
    s += gaugeModeToString(gauge[g].mode);
    s += ":";
    s += machineStateToString(gauge[g].machineState);
    s += ":";
    s += String(gauge[g].eventCount);
  }
  return s;
}

String getAllGaugeConfigString() {
  String s = "GAUGE_CONFIG";
  for (uint8_t g = 0; g < NUM_GAUGES; g++) {
    s += ",";
    s += String(g + 1);
    s += ":";
    s += gaugeModeToString(gauge[g].mode);
    s += ":A";
    s += String(gaugeReedAIndex(g));
    s += ":B";
    s += String(gaugeReedBIndex(g));
  }
  return s;
}