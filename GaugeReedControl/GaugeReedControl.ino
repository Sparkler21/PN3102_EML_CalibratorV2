#include <Arduino.h>
#include <Ethernet.h>
#include "GaugeReedControl.h"

// ------------------------------------------------------------
// Network configuration
// ------------------------------------------------------------

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(172, 16, 168, 99);
const uint16_t TCP_PORT = 5000;

EthernetServer server(TCP_PORT);
EthernetClient client;

// ------------------------------------------------------------
// TCP line buffer
// ------------------------------------------------------------

String rxLine = "";

// ------------------------------------------------------------
// Forward declarations
// ------------------------------------------------------------

String handleCommand(const String &rawCmd);
void serviceTcpClient();
void sendResponse(const String &s);

// ------------------------------------------------------------
// Command handler
// ------------------------------------------------------------

String handleCommand(const String &rawCmd) {
  String cmd = rawCmd;
  cmd.trim();

  if (cmd.length() == 0) {
    return "";
  }

  if (cmd.equalsIgnoreCase("PING")) {
    return "PONG";
  }

  if (cmd.equalsIgnoreCase("GET_GAUGE_CONFIG")) {
    return getAllGaugeConfigString();
  }

  if (cmd.equalsIgnoreCase("GET_GAUGE_STATUS")) {
    return getAllGaugeStatusString();
  }

  if (cmd.equalsIgnoreCase("RESET_REED_STATS")) {
    resetAllReedStats();
    return "OK,RESET_REED_STATS";
  }

  if (cmd.equalsIgnoreCase("RESET_GAUGE_STATS")) {
    resetAllGaugeStats();
    return "OK,RESET_GAUGE_STATS";
  }

  if (cmd.equalsIgnoreCase("RESET_FAULTS")) {
    resetAllGaugeFaults();
    return "OK,RESET_FAULTS";
  }

  // GET_GAUGE_STATUS,<gauge>
  if (cmd.startsWith("GET_GAUGE_STATUS,")) {
    int comma = cmd.indexOf(',');
    int gaugeNum = cmd.substring(comma + 1).toInt();

    if (gaugeNum < 1 || gaugeNum > NUM_GAUGES) {
      return "ERROR,BAD_GAUGE";
    }

    return getGaugeStatusString(gaugeNum - 1);
  }

  // GET_REED_STATUS,<reed>
  if (cmd.startsWith("GET_REED_STATUS,")) {
    int comma = cmd.indexOf(',');
    int reedNum = cmd.substring(comma + 1).toInt();

    if (reedNum < 1 || reedNum > NUM_REEDS) {
      return "ERROR,BAD_REED";
    }

    return getReedStatusString(reedNum - 1);
  }

  // SET_GAUGE_MODE,<gauge>,<mode>
  // mode: 0=DISABLED, 1=SINGLE_A, 2=DUAL
  if (cmd.startsWith("SET_GAUGE_MODE,")) {
    int c1 = cmd.indexOf(',');
    int c2 = cmd.indexOf(',', c1 + 1);

    if (c1 < 0 || c2 < 0) {
      return "ERROR,BAD_FORMAT";
    }

    String gaugeStr = cmd.substring(c1 + 1, c2);
    String modeStr  = cmd.substring(c2 + 1);

    gaugeStr.trim();
    modeStr.trim();

    int gaugeNum = gaugeStr.toInt();
    int modeVal  = modeStr.toInt();

    if (gaugeNum < 1 || gaugeNum > NUM_GAUGES) {
      return "ERROR,BAD_GAUGE";
    }

    if (!setGaugeModeFromInt(gaugeNum, modeVal)) {
      return "ERROR,BAD_MODE";
    }

    return "OK,SET_GAUGE_MODE," + String(gaugeNum) + "," +
           String(gaugeModeToString(getGaugeMode(gaugeNum)));
  }

  return "ERROR,UNKNOWN_COMMAND";
}

// ------------------------------------------------------------
// TCP helpers
// ------------------------------------------------------------

void sendResponse(const String &s) {
  if (client && client.connected() && s.length() > 0) {
    client.println(s);
  }
}

void serviceTcpClient() {
  // Accept a new client if needed
  if (!client || !client.connected()) {
    EthernetClient newClient = server.available();
    if (newClient) {
      client = newClient;
      rxLine = "";
      client.println("OK,CONNECTED");
    }
    return;
  }

  // Read incoming characters
  while (client.available() > 0) {
    char c = (char)client.read();

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      String line = rxLine;
      rxLine = "";
      line.trim();

      if (line.length() > 0) {
        String response = handleCommand(line);
        if (response.length() > 0) {
          sendResponse(response);
        }
      }
    }
    else {
      rxLine += c;

      // avoid runaway memory use
      if (rxLine.length() > 120) {
        rxLine = "";
        sendResponse("ERROR,LINE_TOO_LONG");
      }
    }
  }

  // If client disconnected, tidy up
  if (!client.connected()) {
    client.stop();
    rxLine = "";
  }
}

// ------------------------------------------------------------
// Setup / loop
// ------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(500);

  initGaugeReedControl();

  // Example startup configuration
  setGaugeMode(1, GAUGE_DUAL);
  setGaugeMode(2, GAUGE_SINGLE_A);
  setGaugeMode(3, GAUGE_DISABLED);
  setGaugeMode(4, GAUGE_DUAL);

  Ethernet.begin(mac, ip);
  delay(1000);
  server.begin();

  Serial.println("Gauge/Reed TCP control ready");
  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());
  Serial.print("Port: ");
  Serial.println(TCP_PORT);
  Serial.println(getAllGaugeConfigString());
}

void loop() {
  updateGaugeReedControl();
  serviceTcpClient();
}