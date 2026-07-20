#ifndef GLITTER_COMMAND_H
#define GLITTER_COMMAND_H

#include "LightCommand.h"

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

#endif // GLITTER_COMMAND_H
