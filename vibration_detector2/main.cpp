#include <Arduino.h>
#include <esp_adc_cal.h>

#include "LightInstrument.h"
#include "VibrationDetector.h"

#define DEVICE_RESET D7
#define DETECTOR_PIN A1

constexpr uint32_t SAMPLE_RATE_HZ = 1000;
constexpr uint32_t SAMPLE_INTERVAL_US = 1000000 / SAMPLE_RATE_HZ;
constexpr uint32_t DEBOUNCE_MS = 100;

LightInstrument device;
VibrationDetector detector;

uint32_t lastSampleMicros = 0;
uint32_t lastTriggerMillis = 0;

void setup() {
  device.begin(DEVICE_RESET);

  if (device.getDeviceName() == "") {
    Serial.println("No persistent name found. Waiting for name=... command via Serial.");

    while (device.getDeviceName() == "") {
      device.listenForDeviceName();
      delay(100);
    }
  }

  device.signalBootStart();
  detector.begin(DETECTOR_PIN);

  Serial.println("Waiting for relay discovery...");

  while (!device.isRelayFound()) {
    device.update();
    delay(10);
  }

  Serial.println("Relay discovered!");
  device.signalDeviceReady();
}

void loop() {
  device.update();

  uint32_t currentMicros = micros();

  if (currentMicros - lastSampleMicros >= SAMPLE_INTERVAL_US) {
    lastSampleMicros = currentMicros;

    uint16_t rawValue = analogRead(DETECTOR_PIN);
    float currentEnvelope = detector.update(rawValue);

    if (detector.isVibrating() && (millis() - lastTriggerMillis > DEBOUNCE_MS)) {
      lastTriggerMillis = millis();

      device.sendEvent("A1", currentEnvelope);
      Serial.printf("%d %4.3f\n", rawValue, currentEnvelope);
    }
  }
}
