#include <Arduino.h>
#include "LightInstrument.h"

#define DEVICE_RESET D7
#define BTN_PRESSED 1
#define BTN_RELEASED 0

LightInstrument device;

const int buttonPins[] = {D1, D2, D3};
const int numButtons = 3;
char currentPortName[3];

// State tracking
int lastStates[] = {HIGH, HIGH, HIGH};

void setup() {
  device.begin(DEVICE_RESET);

  if (device.getDeviceName() == "") {
    Serial.println("No persistent name found. Waiting for name=... command via Serial.");

    while (device.getDeviceName() == "") {
      device.listenForDeviceConfig();
      delay(100);
    }
  }

  device.signalBootStart();

  for (int i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  Serial.println("Waiting for relay discovery...");

  while (!device.isRelayFound()) {
    delay(10);
  }

  Serial.println("Relay discovered!");
  device.signalDeviceReady();
}

void loop() {
  device.update();

  for (int i = 0; i < numButtons; i++) {
    int currentState = digitalRead(buttonPins[i]);

    if (currentState != lastStates[i]) {
      sprintf(currentPortName, "D%d", i+1);

      if (currentState == LOW) {
        device.sendEvent(currentPortName, BTN_RELEASED);
      } else {
        device.sendEvent(currentPortName, BTN_PRESSED);
      }

      lastStates[i] = currentState;
    }
  }

  delay(10);
}
