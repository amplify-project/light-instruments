#include "FireCommand.h"
#include "../Globals.h"
#include <Arduino.h>

void FireCommand::execute(const CommandPacket& packet) {
  const char* value = packet.value;

  if (value) {
    int r, g, b, intensity;

    if (sscanf(value, "%d,%d,%d,%d", &r, &g, &b, &intensity) == 4) {
      CRGB color = CRGB(r, g, b);

      for (int i = 0; i < numLedStrips; i++) {
        if (isTargetPort(packet, ledStrips[i].name)) {
          ledStrips[i].activeAnimation = std::unique_ptr<Animation>(new FireAnimation(color, (uint8_t)intensity));
        }
      }
    }
  }
}
