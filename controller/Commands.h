#ifndef COMMANDS_H
#define COMMANDS_H

#include "LightCommand.h"

class SetCommand : public LightCommand {
public:
    void execute(const JsonDocument& doc) override;
};

class SetColorCommand : public LightCommand {
public:
    void execute(const JsonDocument& doc) override;
};

class PulseCommand : public LightCommand {
public:
    void execute(const JsonDocument& doc) override;
    void update() override;

private:
    struct PulseAnmin {
        bool active = false;
        char targetPort[16] = ""; // Empty string means all ports
        uint32_t startTime = 0;
        uint16_t attack = 0;  // ms
        uint16_t decay = 0;   // ms
        uint16_t sustain = 0; // ms
        uint16_t release = 0; // ms
        uint8_t targetBrightness = 255;
        CRGB color = CRGB::White;
    } pulseAnim;
};

class SetBrightnessCommand : public LightCommand {
public:
    void execute(const JsonDocument& doc) override;
};

class CometCommand : public LightCommand {
public:
    void execute(const JsonDocument& doc) override;
    void update() override;

private:
    struct CometAnim {
        bool active = false;
        char targetPort[16] = "";
        CRGB color = CRGB::White;
        uint32_t speed = 0;
        uint32_t lastUpdate = 0;
        float position = 0;
    } cometAnim;
};

class GlitterCommand : public LightCommand {
public:
    void execute(const JsonDocument& doc) override;
    void update() override;

private:
    struct GlitterAnim {
        bool active = false;
        char targetPort[16] = "";
        CRGB color = CRGB::White;
        uint32_t duration = 0;
        uint32_t startTime = 0;
        uint32_t lastUpdate = 0;
    } glitterAnim;
};
    
class RainbowCommand : public LightCommand {
public:
    void execute(const JsonDocument& doc) override;
    void update() override;

private:
    struct RainbowAnim {
        bool active = false;
        char targetPort[16] = "";
        uint8_t initialHue = 0;
        uint8_t deltaHue = 5;
        uint32_t lastUpdate = 0;
    } rainbowAnim;
};

#endif // COMMANDS_H
