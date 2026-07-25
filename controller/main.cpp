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
#include "commands/FireCommand.h"
#include "commands/SetLEDCommand.h"

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

  if (!initPersistentConfig()) {
    Serial.println("Persistent configuration missing. Waiting for name=... and numleds=... commands via Serial.");

    while (!listenForSerialConfig()) {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }

  Serial.print("Device Name: ");
  Serial.println(deviceName);
  Serial.print("LEDs per strip: ");
  Serial.println(numLedsPerStrip);

  pinMode(LED_BUILTIN, OUTPUT);
  flashBuiltinLed();

  // Add LED strips here
  addLedStrip(LED1, numLedsPerStrip, "LED1");
  addLedStrip(LED2, numLedsPerStrip, "LED2");
  addLedStrip(LED3, numLedsPerStrip, "LED3");
  addLedStrip(LED4, numLedsPerStrip, "LED4");

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
  commandManager.registerCommand("fire", std::unique_ptr<LightCommand>(new FireCommand()));
  commandManager.registerCommand("setLED", std::unique_ptr<LightCommand>(new SetLEDCommand()));

  setupWireless();
  digitalWrite(LED_BUILTIN, LOW);

  xTaskCreatePinnedToCore(wirelessTask, "WirelessTask", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(animationTask, "AnimationTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(displayTask, "DisplayTask", 4096, NULL, 1, &displayTaskHandle, 1);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(100));
}
