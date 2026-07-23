#include "RainbowCommand.h"
#include "../Globals.h"
#include <Arduino.h>
#include <cstring>

void RainbowCommand::execute(const CommandPacket& packet) {
  const char* value = packet.value;
  uint8_t deltaHue = 5;

  if (value) {
    int dHue;

    if (sscanf(value, "%d", &dHue) == 1) {
      deltaHue = (uint8_t)dHue;
    }
  }

  for (int i = 0; i < numLedStrips; i++) {
    if (isTargetPort(packet, ledStrips[i].name)) {
      ledStrips[i].activeAnimation = std::unique_ptr<Animation>(new RainbowAnimation(deltaHue));
    }
  }
}
