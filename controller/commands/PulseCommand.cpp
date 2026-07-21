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

            if (port) {
                strncpy(pulseAnim.targetPort, port, sizeof(pulseAnim.targetPort) - 1);
                pulseAnim.targetPort[sizeof(pulseAnim.targetPort) - 1] = '\0';
            } else {
                pulseAnim.targetPort[0] = '\0';
            }

            pulseAnim.active = true;
        }
    }
}

void PulseCommand::update() {
    if (!pulseAnim.active) return;

    uint32_t now = millis();
    uint32_t elapsed = now - pulseAnim.startTime;
    uint8_t brightness = 0;

    if (elapsed < pulseAnim.attack) {
        brightness = map(elapsed, 0, pulseAnim.attack, 0, 255);
    } else if (elapsed < pulseAnim.attack + pulseAnim.decay) {
        brightness = map(elapsed - pulseAnim.attack, 0, pulseAnim.decay, 255, 200);
    } else if (elapsed < pulseAnim.attack + pulseAnim.decay + pulseAnim.sustain) {
        brightness = 200;
    } else if (elapsed < pulseAnim.attack + pulseAnim.decay + pulseAnim.sustain + pulseAnim.release) {
        brightness = map(elapsed - (pulseAnim.attack + pulseAnim.decay + pulseAnim.sustain), 0, pulseAnim.release, 200, 0);
    } else {
        pulseAnim.active = false;
        brightness = 0;
    }

    for (int i = 0; i < numLedStrips; i++) {
        bool matches = (pulseAnim.targetPort[0] == '\0' || ledStrips[i].name == pulseAnim.targetPort);

        if (matches) {
            fill_solid(ledStrips[i].leds, ledStrips[i].numLeds, pulseAnim.color);
        }
    }

    FastLED.setBrightness(brightness);
    FastLED.show();
}
