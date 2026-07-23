#include "main.h"

#include "commands/SetCommand.h"
#include "commands/SetColorCommand.h"
#include "commands/PulseCommand.h"
#include "commands/CometCommand.h"
#include "commands/GlitterCommand.h"
#include "commands/RainbowCommand.h"
#include "commands/SetBrightnessCommand.h"
#include "commands/StopCommand.h"
#include "commands/BreatheCommand.h"

// Set device name here
String deviceName = "receiver2";

void wirelessTask(void *pvParameters) {
  for (;;) {
    handlePing();
    processIncomingPackets();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void animationTask(void *pvParameters) {
  for (;;) {
    commandManager.update();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void displayTask(void *pvParameters) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    showAll();
  }
}

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
  commandManager.registerCommand("stop", std::unique_ptr<LightCommand>(new StopCommand()));
  commandManager.registerCommand("breathe", std::unique_ptr<LightCommand>(new BreatheCommand()));

  setupWireless();
  digitalWrite(LED_BUILTIN, LOW);

  xTaskCreatePinnedToCore(wirelessTask, "WirelessTask", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(animationTask, "AnimationTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(displayTask, "DisplayTask", 4096, NULL, 1, &displayTaskHandle, 1);
}

void loop() {
  // Tasks are running in background
  vTaskDelay(pdMS_TO_TICKS(1000));
}
