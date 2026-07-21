#ifndef GLITTER_COMMAND_H
#define GLITTER_COMMAND_H

#include "LightCommand.h"
#include "../animations/GlitterAnimation.h"

class GlitterCommand : public LightCommand {
public:
    void execute(const JsonDocument& doc) override;
    void update() override {}
};

#endif // GLITTER_COMMAND_H
