#include "PulseCommand.h"
#include "../Globals.h"
#include <Arduino.h>
#include <cstring>

void PulseCommand::execute(const CommandPacket& packet) {
  const char* value = packet.value;

  if (value) {
    int r, g, b, a, d, s, re;

    if (sscanf(value, "%d,%d,%d,%d,%d,%d,%d", &r, &g, &b, &a, &d, &s, &re) == 7) {
      CRGB color = CRGB(r, g, b);

      for (int i = 0; i < numLedStrips; i++) {
        if (isTargetPort(packet, ledStrips[i].name)) {
          ledStrips[i].activeAnimation = std::unique_ptr<Animation>(new PulseAnimation(color, a, d, s, re));
        }
      }
    }
  }
}
