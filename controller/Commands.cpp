#include "Commands.h"
#include "Globals.h"
#include <Arduino.h>
#include <cstring>

void SetCommand::execute(const JsonDocument& doc) {
    const char* value = doc["data"];
    if (!value) return;

    bool updated = false;

    for (int i = 0; i < numLedStrips; i++) {
        if (isTargetPort(doc, ledStrips[i].name)) {
            int r, g, b, brightness;

            if (sscanf(value, "%d,%d,%d,%d", &r, &g, &b, &brightness) == 4) {
                fill_solid(ledStrips[i].leds, ledStrips[i].numLeds, CRGB(r, g, b));
                FastLED.setBrightness(brightness);

                updated = true;
            }
        }
    }

    if (updated) {
        FastLED.show();
    }
}

void SetColorCommand::execute(const JsonDocument& doc) {
    const char* value = doc["data"];
    if (!value) return;

    bool updated = false;
    for (int i = 0; i < numLedStrips; i++) {
        if (isTargetPort(doc, ledStrips[i].name)) {
            int r, g, b;

            if (sscanf(value, "%d,%d,%d", &r, &g, &b) == 3) {
                fill_solid(ledStrips[i].leds, ledStrips[i].numLeds, CRGB(r, g, b));
                updated = true;
            }
        }
    }

    if (updated) {
        FastLED.show();
    }
}

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
        bool matches = (pulseAnim.targetPort[0] == '\0' || strcmp(ledStrips[i].name, pulseAnim.targetPort) == 0);

        if (matches) {
            fill_solid(ledStrips[i].leds, ledStrips[i].numLeds, pulseAnim.color);
        }
    }

    FastLED.setBrightness(brightness);
    FastLED.show();
}

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
                strncpy(cometAnim.targetPort, port, sizeof(cometAnim.targetPort) - 1);
                cometAnim.targetPort[sizeof(cometAnim.targetPort) - 1] = '\0';
            } else {
                cometAnim.targetPort[0] = '\0';
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
            bool matches = (cometAnim.targetPort[0] == '\0' || strcmp(ledStrips[i].name, cometAnim.targetPort) == 0);

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
            }
        }

        FastLED.show();

        if (stillRunning) {
            cometAnim.position += 1.0f;
        } else {
            cometAnim.active = false;
        }
    }
}

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
