#include <Arduino.h>

#define DIP1 D3
#define DIP2 D4
#define DIP3 D5

#define SELECTOR1 D0
#define SELECTOR2 D1
#define SELECTOR3 D2

#define DATA D8

int numChannels = 0;

int getNumChannels() {
  int channels = digitalRead(DIP1) | digitalRead(DIP2) << 1 | digitalRead(DIP3) << 2;

  if (channels == 0) {
    return 8;
  }

  return channels;
}

void setup() {
  Serial.begin(115200);

  pinMode(DIP1, INPUT_PULLUP);
  pinMode(DIP2, INPUT_PULLUP);
  pinMode(DIP3, INPUT_PULLUP);

  pinMode(SELECTOR1, OUTPUT);
  pinMode(SELECTOR2, OUTPUT);
  pinMode(SELECTOR3, OUTPUT);

  pinMode(DATA, INPUT);

  numChannels = getNumChannels();
}

void loop() {
  for (int i=0; i<numChannels; i++) {
    digitalWrite(SELECTOR1, (i & 0b001) >> 0);
    digitalWrite(SELECTOR2, (i & 0b010) >> 1);
    digitalWrite(SELECTOR3, (i & 0b100) >> 2);

    delay(10);

    u_int16_t data = analogRead(DATA);
    Serial.printf("%d => %d\n", i, data);
  }
}
