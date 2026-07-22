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
                CRGB newColor(r, g, b);
                ledStrips[i].brightness = (uint8_t)brightness;

                if (ledStrips[i].activeAnimation) {
                    ledStrips[i].activeAnimation->setColor(newColor);
                } else {
                    fill_solid(ledStrips[i].leds, ledStrips[i].numLeds, newColor);
                }

                showStrip(i);
            }
        }
    }
}
