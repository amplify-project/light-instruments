#include <Arduino.h>
#include "LightInstrument.h"

#define DEVICE_RESET D7
#define PHOTODIODE_PIN A3

LightInstrument device;

const int threshold = 50; // Ignore minor voltage jitter
int lastValue = -1;

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

  pinMode(PHOTODIODE_PIN, INPUT);
  analogReadResolution(12);

  Serial.println("Waiting for relay discovery...");

  while (!device.isRelayFound()) {
    delay(10);
  }

  Serial.println("Relay discovered!");
  device.signalDeviceReady();
}

void loop() {
  device.update();

  int currentValue = analogRead(PHOTODIODE_PIN);

  // Only send if the value has changed significantly
  if (abs(currentValue - lastValue) > threshold) {
    device.sendEvent("A3", currentValue);
    lastValue = currentValue;
    Serial.printf("Value: %d\n", currentValue);
  }

  delay(50);
}
