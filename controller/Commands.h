#ifndef COMMANDS_H
#define COMMANDS_H

#include "LightCommand.h"
#include "AnimationState.h"

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
    AnimationState pulseAnim;
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

#endif // COMMANDS_H
