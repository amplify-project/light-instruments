#include <Arduino.h>
#include <Bounce2.h>
#include "LightInstrument.h"

#define DEVICE_RESET D7
#define DEBOUNCE_INTERVAL 10

LightInstrument device;

const int port = D3;
Bounce debouncer = Bounce();

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

  debouncer.attach(port, INPUT_PULLUP);
  debouncer.interval(DEBOUNCE_INTERVAL);

  Serial.println("Waiting for relay discovery...");

  while (!device.isRelayFound()) {
    delay(10);
  }

  Serial.println("Relay discovered!");
  device.signalDeviceReady();
}

void loop() {
  device.update();
  debouncer.update();

  if (debouncer.fell()) {
    device.sendEvent("D3", 1);
    Serial.println("Sent event");
  }
}
