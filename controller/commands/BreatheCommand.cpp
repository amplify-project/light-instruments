#include "BreatheCommand.h"
#include "../Globals.h"
#include <Arduino.h>

void BreatheCommand::execute(const CommandPacket& packet) {
  const char* value = packet.value;

  if (value) {
    int r, g, b, bpm;

    if (sscanf(value, "%d,%d,%d,%d", &r, &g, &b, &bpm) == 4) {
      CRGB color = CRGB(r, g, b);

      for (int i = 0; i < numLedStrips; i++) {
        if (isTargetPort(packet, ledStrips[i].name)) {
          ledStrips[i].activeAnimation = std::unique_ptr<Animation>(new BreatheAnimation(color, (uint8_t)bpm));
        }
      }
    }
  }
}
