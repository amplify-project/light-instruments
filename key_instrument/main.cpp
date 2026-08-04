#include <Arduino.h>
#include "LightInstrument.h"

#define DEVICE_RESET D7
#define BTN_PRESSED 1
#define BTN_RELEASED 0

LightInstrument device;

const int buttonPins[] = {D1, D2, D3};
const int numButtons = 3;

// State tracking
int lastStates[] = {HIGH, HIGH, HIGH};

void setup() {
  device.begin(DEVICE_RESET);

  if (device.getDeviceName() == "") {
    Serial.println("No persistent name found. Waiting for name=... command via Serial.");

    while (device.getDeviceName() == "") {
      device.listenForDeviceName();
      delay(100);
    }
  }

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);

  for (int i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  Serial.println("Waiting for relay discovery...");

  while (!device.isRelayFound()) {
    delay(10);
  }

  Serial.println("Relay discovered!");
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  device.update();

  for (int i = 0; i < numButtons; i++) {
    int currentState = digitalRead(buttonPins[i]);

    if (currentState != lastStates[i]) {
      if (currentState == LOW) {
        device.sendEvent(String(i).c_str(), BTN_PRESSED);
      } else {
        device.sendEvent(String(i).c_str(), BTN_RELEASED);
      }

      lastStates[i] = currentState;
    }
  }

  delay(10);
}
