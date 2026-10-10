#include <Arduino.h>
#include <esp_adc_cal.h>

#include "AnalogLightInstrument.h"
#include "VibrationDetector.h"

#define DEVICE_RESET D7
#define DETECTOR_PIN A1

constexpr uint32_t SAMPLE_RATE_HZ = 1000;
constexpr uint32_t SAMPLE_INTERVAL_US = 1000000 / SAMPLE_RATE_HZ;
constexpr uint32_t DEBOUNCE_MS = 100;

AnalogLightInstrument device;
VibrationDetector detector;

bool deviceReady = false;
bool isAnalog = false;

uint32_t lastSampleMicros = 0;
uint32_t lastTriggerMillis = 0;

void processingTask(void* pvParameters) {
  for (;;) {
    uint32_t currentMicros = micros();

    if (currentMicros - lastSampleMicros >= SAMPLE_INTERVAL_US) {
      lastSampleMicros = currentMicros;

      uint16_t rawValue = analogRead(DETECTOR_PIN);
      float currentPeak = detector.update(rawValue);

      if (!isAnalog) {
        if (detector.isVibrating() && (millis() - lastTriggerMillis > DEBOUNCE_MS)) {
          lastTriggerMillis = millis();
          device.sendEvent("A1", 1);
        }
      } else {
        lastTriggerMillis = millis();
        device.sendEvent("A1", currentPeak);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void setup() {
  device.begin(DEVICE_RESET);

  if (device.getDeviceName() == "") {
    while (device.getDeviceName() == "") {
      device.listenForDeviceConfig();
      delay(100);
    }
  }

  isAnalog = device.getIsAnalog();
  device.signalBootStart();
  detector.begin(DETECTOR_PIN);
}

void loop() {
  device.update();

  if (!deviceReady) {
    if (!device.isRelayFound()) {
      return;
    }

    device.signalDeviceReady();
    xTaskCreatePinnedToCore(processingTask, "ProcessingTask", 4096, NULL, 1, NULL, 1);
    deviceReady = true;
  }

  vTaskDelay(pdMS_TO_TICKS(200));
}
