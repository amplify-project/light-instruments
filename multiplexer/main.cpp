#include <Arduino.h>
#include "config.h"
#include "LightInstrument.h"

#define DEVICE_RESET D7

LightInstrument device;

int getNumChannels() {
  int channels = !digitalRead(DIP1) | !digitalRead(DIP2) << 1 | !digitalRead(DIP3) << 2;

  if (channels == 0) {
    return 8;
  }

  return channels;
}

void setPort(int i) {
  digitalWrite(SELECTOR1, (i & 0b001) >> 0);
  digitalWrite(SELECTOR2, (i & 0b010) >> 1);
  digitalWrite(SELECTOR3, (i & 0b100) >> 2);
  delay(1);
}

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

  pinMode(DIP1, INPUT_PULLUP);
  pinMode(DIP2, INPUT_PULLUP);
  pinMode(DIP3, INPUT_PULLUP);

  pinMode(SELECTOR1, OUTPUT);
  pinMode(SELECTOR2, OUTPUT);
  pinMode(SELECTOR3, OUTPUT);

  pinMode(DATA, INPUT_PULLDOWN);

  Serial.println("Waiting for relay discovery...");

  while (!device.isRelayFound()) {
    delay(10);
  }

  Serial.println("Relay discovered!");
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  device.update();

  for (int i=0; i<getNumChannels(); i++) {
    setPort(i);

    u_int16_t data = digitalRead(DATA);
    Serial.printf("%d => %d\n", i, data);
    device.sendEvent(String(i).c_str(), data);

    delay(10);
  }
}
