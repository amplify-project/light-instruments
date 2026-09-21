#include <Arduino.h>

#include "StopCommand.h"
#include "../Globals.h"

void StopCommand::execute(const CommandPacket& packet) {
  bool changed = false;

  for (int i = 0; i < numLedStrips; i++) {
    if (isTargetPort(packet, ledStrips[i].name)) {
      // Stop any active animation
      ledStrips[i].activeAnimation.reset();

      // Turn off all LEDs on this strip
      fill_solid(ledStrips[i].leds, ledStrips[i].numLeds, CRGB::Black);

      changed = true;
    }
  }

  if (changed) {
    // Update the hardware via the display task
    triggerDisplay();
  }
}
