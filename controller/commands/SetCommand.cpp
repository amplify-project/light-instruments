#include "SetCommand.h"
#include "../Globals.h"
#include <Arduino.h>

void SetCommand::execute(const JsonDocument& doc) {
    const char* value = doc["data"];
    if (!value) return;

    for (int i = 0; i < numLedStrips; i++) {
        if (isTargetPort(doc, ledStrips[i].name)) {
            int r, g, b, brightness;

            if (sscanf(value, "%d,%d,%d,%d", &r, &g, &b, &brightness) == 4) {
                fill_solid(ledStrips[i].leds, ledStrips[i].numLeds, CRGB(r, g, b));
                ledStrips[i].brightness = (uint8_t)brightness;
                showStrip(i);
            }
        }
    }
}
