#include <Arduino.h>

#include "SetLEDCommand.h"
#include "../Globals.h"

void SetLEDCommand::execute(const CommandPacket& packet) {
  const char* value = packet.value;
  if (!value) return;

  for (int i = 0; i < numLedStrips; i++) {
    if (isTargetPort(packet, ledStrips[i].name)) {
      int r, g, b, offset, numLedsToSet;

      if (sscanf(value, "%d,%d,%d,%d,%d", &r, &g, &b, &offset, &numLedsToSet) == 5) {
        CRGB newColor(r, g, b);

        // Stop active animation as it would overwrite these LEDs
        ledStrips[i].activeAnimation.reset();

        if (offset < 0) offset = 0;

        if (offset < ledStrips[i].numLeds) {
          int end = offset + numLedsToSet;

          if (end > ledStrips[i].numLeds) {
            end = ledStrips[i].numLeds;
          }

          for (int j = offset; j < end; j++) {
            ledStrips[i].leds[j] = newColor;
          }

          showStrip(i);
        }
      }
    }
  }
}
