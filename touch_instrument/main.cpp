#include <Arduino.h>
#include "LightInstrument.h"

#define DEVICE_RESET D7

LightInstrument device;

const int touchPins[] = {D1, D2, D3};
const int touchThreshold = 256; // Threshold for digital touch (0-1023 scale)
uint32_t touchMinima[] = {0, 0, 0};
const uint32_t touchMaxDiff = 30000; // Expected max increase from baseline to reach 1023

// State tracking
int lastSentValues[] = {0, 0, 0};
float filteredValues[] = {0, 0, 0};
const float filterAlpha = 0.7f; // Smoothing factor (0.0 to 1.0), lower is smoother

const String portMapping[] = {"D2", "D3", "D1"};

int processValue(int i, uint32_t val) {
  if (val < touchMinima[i]) {
    return 0;
  }

  uint32_t adjustedVal = val - touchMinima[i];

  if (adjustedVal > touchMaxDiff) {
    adjustedVal = touchMaxDiff;
  }

  return map(adjustedVal, 0, touchMaxDiff, 0, 1023);
}

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

  Serial.println("Calibrating touch sensors (leave them untouched)...");
  delay(1000);

  for (int i = 0; i < 3; i++) {
    uint32_t sum = 0;

    for (int j = 0; j < 10; j++) {
      sum += touchRead(touchPins[i]);
      delay(10);
    }

    touchMinima[i] = sum / 10;
    filteredValues[i] = 0;
    Serial.printf("Pin D%d baseline: %u\n", i + 1, touchMinima[i]);
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

  for (int i = 0; i < 3; i++) {
    uint32_t rawVal = touchRead(touchPins[i]);
    int mappedVal = processValue(i, rawVal);

    filteredValues[i] = (filterAlpha * mappedVal) + ((1.0f - filterAlpha) * filteredValues[i]);
    int finalVal = (int)filteredValues[i];
    int currentState = (finalVal >= touchThreshold) ? 1 : 0;

    if (currentState != lastSentValues[i]) {
      device.sendEvent(portMapping[i].c_str(), currentState);
      lastSentValues[i] = currentState;

      Serial.printf("Port %s | Digital: %d (Raw: %u, Mapped: %d, Filtered: %d)\n", portMapping[i], currentState, rawVal, mappedVal, finalVal);
    }

    delay(10);
  }
}
