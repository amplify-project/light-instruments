#ifndef PULSE_COMMAND_H
#define PULSE_COMMAND_H

#include "LightCommand.h"

class PulseCommand : public LightCommand {
public:
    void execute(const JsonDocument& doc) override;
    void update() override;

private:
    struct PulseAnim {
        bool active = false;
        std::string targetPort = ""; // Empty string means all ports
        uint32_t startTime = 0;
        uint16_t attack = 0;  // ms
        uint16_t decay = 0;   // ms
        uint16_t sustain = 0; // ms
        uint16_t release = 0; // ms
        uint8_t targetBrightness = 255;
        CRGB color = CRGB::White;
    } pulseAnim;
};

#endif // PULSE_COMMAND_H
