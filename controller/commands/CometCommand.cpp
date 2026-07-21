#include "CometCommand.h"
#include "../Globals.h"
#include <Arduino.h>
#include <cstring>

void CometCommand::execute(const JsonDocument& doc) {
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
        uint32_t speed;

        if (sscanf(value, "%d,%d,%d,%u", &r, &g, &b, &speed) == 4) {
            cometAnim.color = CRGB(r, g, b);
            cometAnim.speed = speed;
            cometAnim.lastUpdate = millis() - speed; // Start immediately
            cometAnim.position = 0;

            if (port) {
                cometAnim.targetPort = port;
            } else {
                cometAnim.targetPort = "";
            }

            cometAnim.active = true;
        }
    }
}

void CometCommand::update() {
    if (!cometAnim.active) return;

    uint32_t now = millis();
    if (now - cometAnim.lastUpdate >= cometAnim.speed) {
        cometAnim.lastUpdate = now;
        bool stillRunning = false;

        for (int i = 0; i < numLedStrips; i++) {
            bool matches = (cometAnim.targetPort.empty() || ledStrips[i].name == cometAnim.targetPort);

            if (matches) {
                // Fade existing LEDs to create the tail
                fadeToBlackBy(ledStrips[i].leds, ledStrips[i].numLeds, 64);

                if (cometAnim.position < ledStrips[i].numLeds) {
                    // Set new head
                    ledStrips[i].leds[(int)cometAnim.position] = cometAnim.color;
                    stillRunning = true;
                } else if (cometAnim.position < ledStrips[i].numLeds + 10) {
                    // Allow tail to fade for a few more steps
                    stillRunning = true;
                }

                showStrip(i);
            }
        }

        if (stillRunning) {
            cometAnim.position += 1.0f;
        } else {
            cometAnim.active = false;
        }
    }
}
