#include <Arduino.h>

#include "SetColorCommand.h"
#include "../Globals.h"

void SetColorCommand::execute(const CommandPacket& packet) {
  const char* value = packet.value;
  if (!value) return;

  int r, g, b;
  if (sscanf(value, "%d,%d,%d", &r, &g, &b) != 3) {
    return;
  }

  CRGB newColor(r, g, b);
  bool changed = false;

  for (int i = 0; i < numLedStrips; i++) {
    if (isTargetPort(packet, ledStrips[i].name)) {
      if (ledStrips[i].activeAnimation) {
        ledStrips[i].activeAnimation->setColor(newColor);
      } else {
        fill_solid(ledStrips[i].leds, ledStrips[i].numLeds, newColor);
      }
      changed = true;
    }
  }

  if (changed) {
    triggerDisplay();
  }
}
