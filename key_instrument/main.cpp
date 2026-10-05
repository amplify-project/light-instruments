#include <Arduino.h>
#include "LightInstrument.h"

#define DEVICE_RESET D7
#define BTN_PRESSED 1
#define BTN_RELEASED 0

LightInstrument device;
bool deviceReady = false;

const int buttonPins[] = {D1, D2, D3};
const int numButtons = 3;
char currentPortName[3];

// State tracking
int lastStates[] = {HIGH, HIGH, HIGH};

void processingTask(void* pvParameters) {
  for (;;) {
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

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup() {
  Serial.begin(115200);
  device.begin(DEVICE_RESET);

  if (device.getDeviceName() == "") {
    while (device.getDeviceName() == "") {
      device.listenForDeviceConfig();
      delay(100);
    }
  }

  device.signalBootStart();

  for (int i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT);
    lastStates[i] = digitalRead(buttonPins[i]);
  }
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
