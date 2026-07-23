#include <Arduino.h>
#include <cstring>

#include "GlitterCommand.h"
#include "../Globals.h"

void GlitterCommand::execute(const CommandPacket& packet) {
  const char* value = packet.value;

  if (value) {
    int r, g, b;
    uint32_t duration = 0;

    if (sscanf(value, "%d,%d,%d,%u", &r, &g, &b, &duration) >= 3) {
      CRGB color = CRGB(r, g, b);

      for (int i = 0; i < numLedStrips; i++) {
        if (isTargetPort(packet, ledStrips[i].name)) {
          ledStrips[i].activeAnimation = std::unique_ptr<Animation>(new GlitterAnimation(color, duration));
        }
      }
    }
  }
}
