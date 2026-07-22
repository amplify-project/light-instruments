#include "SetBrightnessCommand.h"
#include "../Globals.h"
#include <Arduino.h>

void SetBrightnessCommand::execute(const JsonDocument& doc) {
  const char* value = doc["data"];
  if (!value) return;

  int brightness = atoi(value);

  for (int i = 0; i < numLedStrips; i++) {
    if (isTargetPort(doc, ledStrips[i].name)) {
      ledStrips[i].brightness = brightness;
      showStrip(i);
    }
  }
}
