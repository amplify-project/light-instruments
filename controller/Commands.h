#ifndef COMMANDS_H
#define COMMANDS_H

#include "LightCommand.h"
#include "AnimationState.h"

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

#endif // COMMANDS_H
