#include "SetColorCommand.h"
#include "../Globals.h"
#include <Arduino.h>

void SetColorCommand::execute(const JsonDocument& doc) {
    const char* value = doc["data"];
    if (!value) return;

    for (int i = 0; i < numLedStrips; i++) {
        if (isTargetPort(doc, ledStrips[i].name)) {
            int r, g, b;

            if (sscanf(value, "%d,%d,%d", &r, &g, &b) == 3) {
                ledStrips[i].activeAnimation.reset();

                fill_solid(ledStrips[i].leds, ledStrips[i].numLeds, CRGB(r, g, b));
                showStrip(i);
            }
        }
    }
}
