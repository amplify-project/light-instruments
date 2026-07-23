#include "CometCommand.h"
#include "../Globals.h"
#include <Arduino.h>
#include <cstring>

void CometCommand::execute(const CommandPacket& packet) {
  const char* value = packet.value;

  if (value) {
    int r, g, b;
    uint32_t speed;

    if (sscanf(value, "%d,%d,%d,%u", &r, &g, &b, &speed) == 4) {
      CRGB color = CRGB(r, g, b);

      for (int i = 0; i < numLedStrips; i++) {
        if (isTargetPort(packet, ledStrips[i].name)) {
          ledStrips[i].activeAnimation = std::unique_ptr<Animation>(new CometAnimation(color, speed));
        }
      }
    }
  }
}
