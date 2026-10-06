#include <Arduino.h>
#include <Bounce2.h>
#include "LightInstrument.h"

#define DEVICE_RESET D7
#define DEBOUNCE_INTERVAL 10

LightInstrument device;
bool deviceReady = false;

const int port = D3;
Bounce debouncer = Bounce();

void processingTask(void* pvParameters) {
  for (;;) {
    debouncer.update();

    if (debouncer.fell()) {
      device.sendEvent("D3", 1);
    }

    vTaskDelay(pdMS_TO_TICKS(10));
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

  debouncer.attach(port, INPUT_PULLUP);
  debouncer.interval(DEBOUNCE_INTERVAL);
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
