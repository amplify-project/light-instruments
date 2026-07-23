#include "SetBrightnessCommand.h"
#include "../Globals.h"
#include <Arduino.h>

void SetBrightnessCommand::execute(const CommandPacket& packet) {
  const char* value = packet.value;
  if (!value) return;

  int brightness = atoi(value);

  for (int i = 0; i < numLedStrips; i++) {
    if (isTargetPort(packet, ledStrips[i].name)) {
      ledStrips[i].brightness = brightness;
      showStrip(i);
    }
  }
}
