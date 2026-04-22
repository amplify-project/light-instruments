#include "display.h"

void Display::init() {
  reset();
  SSD1306::init();
}

void Display::reset() {
  pinMode(this->rst, OUTPUT);

  digitalWrite(this->rst, LOW);
  delay(50);
  digitalWrite(this->rst, HIGH);
}
