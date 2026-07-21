#ifndef GLOBALS_H
#define GLOBALS_H

#include <FastLED.h>
#include <vector>
#include <string>
#include <memory>
#include "animations/Animation.h"

#define LEDSTRIP(port, obj, num_leds) {#port, obj, port, num_leds}

#define LED1 D0
#define LED2 D1
#define LED3 D2
#define LED4 D3

struct LedStrip {
    std::string name;
    CRGB* leds;
    uint8_t port;
    int numLeds;
    uint8_t brightness = 255;
    CLEDController* controller = nullptr;
    std::unique_ptr<Animation> activeAnimation;

    LedStrip(std::string n, CRGB* l, uint8_t p, int nl, uint8_t b = 255, CLEDController* c = nullptr)
        : name(n), leds(l), port(p), numLeds(nl), brightness(b), controller(c), activeAnimation(nullptr) {}
};

class CommandManager;
class LightCommand;
extern std::vector<LedStrip> ledStrips;
extern CommandManager commandManager;

#define numLedStrips ((int)ledStrips.size())

inline void showStrip(int index) {
    if (index >= 0 && index < numLedStrips) {
        if (ledStrips[index].controller) {
            ledStrips[index].controller->showLeds(ledStrips[index].brightness);
        }
    }
}

inline void showAll() {
    for (int i = 0; i < numLedStrips; i++) {
        showStrip(i);
    }
}

void addLedStrip(uint8_t port, int numLeds, const char* name = nullptr);

#endif // GLOBALS_H
