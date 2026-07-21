#include "PulseCommand.h"
#include "../Globals.h"
#include <Arduino.h>
#include <cstring>

void PulseCommand::execute(const JsonDocument& doc) {
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
        int r, g, b, a, d, s, re;

        if (sscanf(value, "%d,%d,%d,%d,%d,%d,%d", &r, &g, &b, &a, &d, &s, &re) == 7) {
            pulseAnim.color = CRGB(r, g, b);
            pulseAnim.attack = a;
            pulseAnim.decay = d;
            pulseAnim.sustain = s;
            pulseAnim.release = re;
            pulseAnim.startTime = millis();
            pulseAnim.lastUpdate = 0;

            if (port) {
                pulseAnim.targetPort = port;
            } else {
                pulseAnim.targetPort = "";
            }

            pulseAnim.active = true;
        }
    }
}

void PulseCommand::update() {
    if (!pulseAnim.active) return;

    uint32_t now = millis();
    if (now - pulseAnim.lastUpdate < 20) return;
    pulseAnim.lastUpdate = now;

    uint32_t elapsed = now - pulseAnim.startTime;
    uint8_t brightness = 0;

    uint32_t attackEnd = pulseAnim.attack;
    uint32_t decayEnd = attackEnd + pulseAnim.decay;
    uint32_t sustainEnd = decayEnd + pulseAnim.sustain;
    uint32_t releaseEnd = sustainEnd + pulseAnim.release;

    if (elapsed < attackEnd) {
        brightness = (pulseAnim.attack > 0) ? map(elapsed, 0, pulseAnim.attack, 0, 255) : 255;
    } else if (elapsed < decayEnd) {
        brightness = (pulseAnim.decay > 0) ? map(elapsed - attackEnd, 0, pulseAnim.decay, 255, 200) : 200;
    } else if (elapsed < sustainEnd) {
        brightness = 200;
    } else if (elapsed < releaseEnd) {
        brightness = (pulseAnim.release > 0) ? map(elapsed - sustainEnd, 0, pulseAnim.release, 200, 0) : 0;
    } else {
        pulseAnim.active = false;
        brightness = 0;
    }

    CRGB scaledColor = pulseAnim.color;
    scaledColor.nscale8_video(brightness);

    for (int i = 0; i < numLedStrips; i++) {
        bool matches = (pulseAnim.targetPort.empty() || ledStrips[i].name == pulseAnim.targetPort);

        if (matches && ledStrips[i].leds) {
            fill_solid(ledStrips[i].leds, ledStrips[i].numLeds, scaledColor);
        }
    }

    showAll();
}
