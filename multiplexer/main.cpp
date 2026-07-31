#include <Arduino.h>
#include "config.h"

int getNumChannels() {
  int channels = !digitalRead(DIP1) | !digitalRead(DIP2) << 1 | !digitalRead(DIP3) << 2;

  if (channels == 0) {
    return 8;
  }

  return channels;
}

void setup() {
  #ifdef XIAO
  Serial.begin(115200);
  #endif

  #ifdef LOLIN
  Serial.begin(460800);
  #endif

  pinMode(DIP1, INPUT_PULLUP);
  pinMode(DIP2, INPUT_PULLUP);
  pinMode(DIP3, INPUT_PULLUP);

  pinMode(SELECTOR1, OUTPUT);
  pinMode(SELECTOR2, OUTPUT);
  pinMode(SELECTOR3, OUTPUT);

  pinMode(DATA, INPUT_PULLDOWN);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  for (int i=0; i<getNumChannels(); i++) {
    digitalWrite(SELECTOR1, (i & 0b001) >> 0);
    digitalWrite(SELECTOR2, (i & 0b010) >> 1);
    digitalWrite(SELECTOR3, (i & 0b100) >> 2);
    delay(1);

    u_int16_t data = digitalRead(DATA);
    Serial.printf("%d => %d\n", i, data);
    delay(1);
  }

  Serial.println("===");
  delay(1000);
}
