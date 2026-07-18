#include "Commands.h"
#include "Globals.h"
#include <Arduino.h>

void SetColorCommand::execute(const JsonDocument& doc) {
    const char* value = doc["value"];

    if (value && isTargetPort(doc, "D0")) {
        int r, g, b;

        if (sscanf(value, "%d,%d,%d", &r, &g, &b) == 3) {
            fill_solid(ledsD0, NUM_LEDS_D0, CRGB(r, g, b));
            FastLED.show();
        }
    }
}

void PulseCommand::execute(const JsonDocument& doc) {
    const char* value = doc["value"];

    if (value && isTargetPort(doc, "D0")) {
        int r, g, b, a, d, s, re;

        if (sscanf(value, "%d,%d,%d,%d,%d,%d,%d", &r, &g, &b, &a, &d, &s, &re) == 7) {
            pulseAnim.color = CRGB(r, g, b);
            pulseAnim.attack = a;
            pulseAnim.decay = d;
            pulseAnim.sustain = s;
            pulseAnim.release = re;
            pulseAnim.startTime = millis();
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

    fill_solid(ledsD0, NUM_LEDS_D0, pulseAnim.color);
    FastLED.setBrightness(brightness);
    FastLED.show();
}

void SetBrightnessCommand::execute(const JsonDocument& doc) {
    const char* value = doc["value"];

    if (value && isTargetPort(doc, "D0")) {
        FastLED.setBrightness(atoi(value));
        FastLED.show();
    }
}
