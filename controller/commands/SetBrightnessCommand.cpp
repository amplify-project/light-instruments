#include <Arduino.h>

#include "SetBrightnessCommand.h"
#include "../Globals.h"

void SetBrightnessCommand::execute(const CommandPacket& packet) {
  const char* value = packet.value;
  if (!value) return;

  int brightness = atoi(value);
  bool changed = false;

  for (int i = 0; i < numLedStrips; i++) {
    if (isTargetPort(packet, ledStrips[i].name)) {
      ledStrips[i].brightness = brightness;
      changed = true;
    }
  }

  if (changed) {
    triggerDisplay();
  }
}
