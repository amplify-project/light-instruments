#ifndef LIGHT_COMMAND_H
#define LIGHT_COMMAND_H

#include <ArduinoJson.h>
#include <FastLED.h>

class LightCommand {
public:
    virtual ~LightCommand() = default;
    virtual void execute(const JsonDocument& doc) = 0;
    virtual void update() {}

protected:
    bool isTargetPort(const JsonDocument& doc, const char* target) {
        const char* port = doc["port"];
        return port == nullptr || strcmp(port, target) == 0;
    }
};

#endif // LIGHT_COMMAND_H
