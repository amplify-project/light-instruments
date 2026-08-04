#include <Arduino.h>
#include "LightInstrument.h"

#define DEVICE_RESET D7

LightInstrument device;

const int analogPin = A1;
const int threshold = 5; // Ignore minor voltage jitter

// State tracking
float smoothedValue = 0;
const float alpha = 0.6; // Smoothing factor (0.0 to 1.0). Lower = more smoothing, slower response.
int lastValue = -1;

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

  pinMode(analogPin, INPUT);
  analogReadResolution(12);

  Serial.println("Waiting for relay discovery...");

  while (!device.isRelayFound()) {
    device.update();
    delay(10);
  }

  Serial.println("Relay discovered!");
  digitalWrite(LED_BUILTIN, LOW); // Turn on LED (active-low)
}

void loop() {
  device.update();

  int currentValue = analogRead(analogPin);

  // Apply exponential smoothing
  smoothedValue = (alpha * currentValue) + ((1.0 - alpha) * smoothedValue);
  int finalValue = (int)smoothedValue;

  // Only send if the value has changed significantly
  if (abs(finalValue - lastValue) > threshold) {
    device.sendEvent("A1", finalValue);
    lastValue = finalValue;

    Serial.printf("Value: %d\n", finalValue);
  }

  delay(20);
}
