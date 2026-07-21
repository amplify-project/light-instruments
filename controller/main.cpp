#include "main.h"

#include "commands/SetCommand.h"
#include "commands/SetColorCommand.h"
#include "commands/PulseCommand.h"
#include "commands/CometCommand.h"
#include "commands/GlitterCommand.h"
#include "commands/RainbowCommand.h"
#include "commands/SetBrightnessCommand.h"

// Set device name here
String deviceName = "receiver1";

std::vector<LedStrip> ledStrips;
CommandManager commandManager;

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  flashBuiltinLed();

  // Add LED strips here
  addLedStrip(LED1, 30, "LED1");

  for (int i=0; i<numLedStrips; i++) {
    ledStrips[i].brightness = 50;
  }
  FastLED.clear();
  showAll();

  // Register commands here
  commandManager.registerCommand("set", std::unique_ptr<SetCommand>(new SetCommand()));
  commandManager.registerCommand("setColor", std::unique_ptr<LightCommand>(new SetColorCommand()));
  commandManager.registerCommand("pulse", std::unique_ptr<LightCommand>(new PulseCommand()));
  commandManager.registerCommand("comet", std::unique_ptr<LightCommand>(new CometCommand()));
  commandManager.registerCommand("glitter", std::unique_ptr<LightCommand>(new GlitterCommand()));
  commandManager.registerCommand("rainbow", std::unique_ptr<LightCommand>(new RainbowCommand()));
  commandManager.registerCommand("setBrightness", std::unique_ptr<LightCommand>(new SetBrightnessCommand()));

  setupWireless();
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  handlePing();
  processIncomingPackets();
  commandManager.update();
}
