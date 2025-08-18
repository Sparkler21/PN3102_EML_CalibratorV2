/*
 * Portenta Machine Control - Analog out
 *
 * This example demonstrates the utilization of the Analog out channels on the Machine Control.
 * The example configures the channels' PWM period in the setup and then iterates through voltage
 * output values from 0V to 10.5V in a loop.
 *
 * The circuit:
 *  - Portenta H7
 *  - Portenta Machine Control
 *
 * This example code is in the public domain. 
 * Copyright (c) 2024 Arduino
 * SPDX-License-Identifier: MPL-2.0
 */

#include <Arduino_PortentaMachineControl.h>

#define PERIOD_MS 4 /* 4ms - 250Hz */

#define PULSE_INPUT_PIN D2

volatile unsigned long pulseCount = 0;  // Stores pulse count

int needleValve0 = 0;
//int needleValve1 = 0;
//int needleValve2 = 0;
//int needleValve3 = 0;

float voltage = 0;

uint16_t readings = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect.
  }

  pinMode(PULSE_INPUT_PIN, INPUT_PULLUP);  // Set as input
  attachInterrupt(digitalPinToInterrupt(PULSE_INPUT_PIN), handlePulse, FALLING);

  MachineControl_AnalogOut.begin();

  MachineControl_AnalogOut.setPeriod(needleValve0, PERIOD_MS); 
//  MachineControl_AnalogOut.setPeriod(needleValve1, PERIOD_MS);
//  MachineControl_AnalogOut.setPeriod(needleValve2, PERIOD_MS);
//  MachineControl_AnalogOut.setPeriod(needleValve3, PERIOD_MS);


}

void loop() {
  MachineControl_AnalogOut.write(needleValve0, voltage);
//  MachineControl_AnalogOut.write(needleValve1, voltage);
//  MachineControl_AnalogOut.write(needleValve2, voltage);
//  MachineControl_AnalogOut.write(needleValve3, voltage);

  Serial.println("needleValve0 set at " + String(voltage) + "V");
  
  voltage = voltage + 0.1;
  /* Maximum output value is 10.0V */
  if (voltage >= 10.0) {
    voltage = 0;
    delay(100); /* Additional 100 ms delay introduced to manage 10.0V -> 0V fall time of 150 ms */
  }

  delay(100);
    Serial.print("Pulses counted: ");
    Serial.println(pulseCount);
}

// Interrupt Service Routine (ISR) - runs on each rising edge
void handlePulse() {
  pulseCount++;
      Serial.print("INT!");
}