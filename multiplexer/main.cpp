#include <Arduino.h>
#include "config.h"
#include "LightInstrument.h"

#define DEVICE_RESET D7

LightInstrument device;
bool deviceReady = false;

int lastStates[] = { LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW };
char currentPortName[3];
int numChannels;

int getNumChannels() {
  int channels = !digitalRead(DIP1) | !digitalRead(DIP2) << 1 | !digitalRead(DIP3) << 2;

  if (channels == 0) {
    return 8;
  }

  return channels;
}

int readFromPort(int i) {
  digitalWrite(SELECTOR1, (i & 0b001) >> 0);
  digitalWrite(SELECTOR2, (i & 0b010) >> 1);
  digitalWrite(SELECTOR3, (i & 0b100) >> 2);
  delay(1);

  return digitalRead(DATA);
}

void processingTask(void* pvParameters) {
  for (;;) {
    for (int i=0; i<numChannels; i++) {
      int currentState = readFromPort(i);

      if (currentState != lastStates[i]) {
        Serial.printf("D%d => %d\n", i + 1, currentState);

        sprintf(currentPortName, "D%d", i + 1);
        device.sendEvent(currentPortName, currentState);

        lastStates[i] = currentState;
      }

      vTaskDelay(pdMS_TO_TICKS(5));
    }
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

  pinMode(DIP1, INPUT_PULLUP);
  pinMode(DIP2, INPUT_PULLUP);
  pinMode(DIP3, INPUT_PULLUP);

  pinMode(SELECTOR1, OUTPUT);
  pinMode(SELECTOR2, OUTPUT);
  pinMode(SELECTOR3, OUTPUT);

  pinMode(DATA, INPUT_PULLDOWN);

  numChannels = getNumChannels();
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
