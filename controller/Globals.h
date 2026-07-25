#ifndef GLOBALS_H
#define GLOBALS_H

#include <vector>

#include "HardwareConfig.h"
#include "LedStrip.h"

class CommandManager;

extern std::vector<LedStrip> ledStrips;
extern CommandManager commandManager;
extern TaskHandle_t displayTaskHandle;
extern String deviceName;
extern String deviceType;
extern int numLedsPerStrip;

#define numLedStrips ((int)ledStrips.size())

void showStrip(int index);
void showAll();
void triggerDisplay();
void addLedStrip(uint8_t port, int numLeds, const char* name = nullptr);
bool initPersistentConfig();
void saveDeviceName(String name);
void saveNumLeds(int numLeds);
bool listenForSerialConfig();

#endif // GLOBALS_H
