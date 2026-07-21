#include "GlitterCommand.h"
#include "../Globals.h"
#include <Arduino.h>
#include <cstring>

void GlitterCommand::execute(const JsonDocument& doc) {
    const char* value = doc["data"];
    const char* port = doc["port"];

    bool anyMatch = false;
    for (int i = 0; i < numLedStrips; i++) {
        if (isTargetPort(doc, ledStrips[i].name)) {
            anyMatch = true;
            break;
        }
    }

    if (value && anyMatch) {
        int r, g, b;
        uint32_t duration;

        if (sscanf(value, "%d,%d,%d,%u", &r, &g, &b, &duration) == 4) {
            glitterAnim.color = CRGB(r, g, b);
            glitterAnim.duration = duration;
            glitterAnim.startTime = millis();
            glitterAnim.lastUpdate = millis() - 20; // Start immediately

            if (port) {
                strncpy(glitterAnim.targetPort, port, sizeof(glitterAnim.targetPort) - 1);
                glitterAnim.targetPort[sizeof(glitterAnim.targetPort) - 1] = '\0';
            } else {
                glitterAnim.targetPort[0] = '\0';
            }

            glitterAnim.active = true;
        }
    }
}

void GlitterCommand::update() {
    if (!glitterAnim.active) return;

    uint32_t now = millis();
    uint32_t elapsed = now - glitterAnim.startTime;

    if (elapsed >= glitterAnim.duration) {
        glitterAnim.active = false;
        return;
    }

    if (now - glitterAnim.lastUpdate >= 20) { // 50 FPS
        glitterAnim.lastUpdate = now;

        for (int i = 0; i < numLedStrips; i++) {
            bool matches = (glitterAnim.targetPort[0] == '\0' || ledStrips[i].name == glitterAnim.targetPort);

            if (matches) {
                // Fade down existing LEDs to make sparkles short-lived
                fadeToBlackBy(ledStrips[i].leds, ledStrips[i].numLeds, 32);

                // Add random sparkles
                if (random8() < 64) { // ~25% chance per frame
                    int pos = random16(ledStrips[i].numLeds);
                    ledStrips[i].leds[pos] += glitterAnim.color;
                }
            }
        }

        FastLED.show();
    }
}
