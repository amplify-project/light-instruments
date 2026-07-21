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
            rainbowAnim.targetPort = port;
        } else {
            rainbowAnim.targetPort = "";
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
            bool matches = (rainbowAnim.targetPort.empty() || ledStrips[i].name == rainbowAnim.targetPort);

            if (matches) {
                fill_rainbow(ledStrips[i].leds, ledStrips[i].numLeds, rainbowAnim.initialHue, rainbowAnim.deltaHue);
            }

            showStrip(i);
        }
    }
}
