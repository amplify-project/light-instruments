#include "RainbowCommand.h"
#include "../Globals.h"
#include <Arduino.h>
#include <cstring>

void RainbowCommand::execute(const JsonDocument& doc) {
    const char* value = doc["data"];
    const char* port = doc["port"];

    bool anyMatch = false;
    for (int i = 0; i < numLedStrips; i++) {
        if (isTargetPort(doc, ledStrips[i].name)) {
            anyMatch = true;
            break;
        }
    }

    if (anyMatch) {
        rainbowAnim.initialHue = 0;
        rainbowAnim.deltaHue = 5; // Default deltaHue

        if (value) {
            int dHue;
            if (sscanf(value, "%d", &dHue) == 1) {
                rainbowAnim.deltaHue = (uint8_t)dHue;
            }
        }

        if (port) {
            strncpy(rainbowAnim.targetPort, port, sizeof(rainbowAnim.targetPort) - 1);
            rainbowAnim.targetPort[sizeof(rainbowAnim.targetPort) - 1] = '\0';
        } else {
            rainbowAnim.targetPort[0] = '\0';
        }

        rainbowAnim.active = true;
        rainbowAnim.lastUpdate = millis();
    }
}

void RainbowCommand::update() {
    if (!rainbowAnim.active) return;

    uint32_t now = millis();
    if (now - rainbowAnim.lastUpdate >= 20) { // 50 FPS
        rainbowAnim.lastUpdate = now;
        rainbowAnim.initialHue++;

        for (int i = 0; i < numLedStrips; i++) {
            bool matches = (rainbowAnim.targetPort[0] == '\0' || strcmp(ledStrips[i].name, rainbowAnim.targetPort) == 0);

            if (matches) {
                fill_rainbow(ledStrips[i].leds, ledStrips[i].numLeds, rainbowAnim.initialHue, rainbowAnim.deltaHue);
            }
        }
        FastLED.show();
    }
}
