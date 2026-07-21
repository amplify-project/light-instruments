#include "GlitterCommand.h"
#include "../Globals.h"
#include <Arduino.h>
#include <cstring>

void GlitterCommand::execute(const JsonDocument& doc) {
    const char* value = doc["data"];

    if (value) {
        int r, g, b;
        uint32_t duration;

        if (sscanf(value, "%d,%d,%d,%u", &r, &g, &b, &duration) == 4) {
            CRGB color = CRGB(r, g, b);

            for (int i = 0; i < numLedStrips; i++) {
                if (isTargetPort(doc, ledStrips[i].name)) {
                    ledStrips[i].activeAnimation = std::unique_ptr<Animation>(new GlitterAnimation(color, duration));
                }
            }
        }
    }
}
