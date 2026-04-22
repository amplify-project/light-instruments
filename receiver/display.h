#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <SSD1306.h>

class Display : public SSD1306 {
  uint8_t rst;

public:
  Display(uint8_t sda, uint8_t scl, uint8_t rst, OLEDDISPLAY_GEOMETRY geometry) : SSD1306(0x3C, sda, scl, geometry), rst(rst) {}

  void init();
  void reset();
};

#endif
