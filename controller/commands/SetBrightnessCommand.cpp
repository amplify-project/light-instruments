#include "SetBrightnessCommand.h"
#include "../Globals.h"
#include <Arduino.h>

void SetBrightnessCommand::execute(const JsonDocument& doc) {
    const char* value = doc["data"];
    if (!value) return;

    bool targetMatches = false;
    for (int i = 0; i < numLedStrips; i++) {
        if (isTargetPort(doc, ledStrips[i].name)) {
            targetMatches = true;
            break;
        }
    }

    if (targetMatches) {
        FastLED.setBrightness(atoi(value));
        FastLED.show();
    }
}
