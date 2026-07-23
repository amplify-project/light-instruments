#include "SetColorCommand.h"
#include "../Globals.h"
#include <Arduino.h>

void SetColorCommand::execute(const CommandPacket& packet) {
  const char* value = packet.value;
  if (!value) return;

  for (int i = 0; i < numLedStrips; i++) {
    if (isTargetPort(packet, ledStrips[i].name)) {
      int r, g, b;

      if (sscanf(value, "%d,%d,%d", &r, &g, &b) == 3) {
        CRGB newColor(r, g, b);

        if (ledStrips[i].activeAnimation) {
          ledStrips[i].activeAnimation->setColor(newColor);
        } else {
          fill_solid(ledStrips[i].leds, ledStrips[i].numLeds, newColor);
          showStrip(i);
        }
      }
    }
  }
}
