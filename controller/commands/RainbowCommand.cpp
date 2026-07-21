#include "RainbowCommand.h"
#include "../Globals.h"
#include <Arduino.h>
#include <cstring>

void RainbowCommand::execute(const JsonDocument& doc) {
    const char* value = doc["data"];
    uint8_t deltaHue = 5;

    if (value) {
        int dHue;

        if (sscanf(value, "%d", &dHue) == 1) {
            deltaHue = (uint8_t)dHue;
        }
    }

    for (int i = 0; i < numLedStrips; i++) {
        if (isTargetPort(doc, ledStrips[i].name)) {
            ledStrips[i].activeAnimation = std::unique_ptr<Animation>(new RainbowAnimation(deltaHue));
        }
    }
}
