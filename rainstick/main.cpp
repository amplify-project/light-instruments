#include <Arduino.h>
#include "LightInstrument.h"

#define DEVICE_RESET D7
#define PHOTODIODE_PIN A3

LightInstrument device;
bool deviceReady = false;

const int threshold = 50; // Ignore minor voltage jitter
int lastValue = -1;

void processingTask(void* pvParameters) {
  for (;;) {
    int currentValue = analogRead(PHOTODIODE_PIN);

    // Only send if the value has changed significantly
    if (abs(currentValue - lastValue) > threshold) {
      device.sendEvent("A3", currentValue);
      lastValue = currentValue;
    }

    vTaskDelay(pdMS_TO_TICKS(50));
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

  device.signalBootStart();

  pinMode(PHOTODIODE_PIN, INPUT);
  analogReadResolution(12);
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

  vTaskDelay(pdMS_TO_TICKS(100));
}
