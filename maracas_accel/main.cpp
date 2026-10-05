#include <Arduino.h>
#include "LightInstrument.h"

#ifdef XIAO
#define DEVICE_RESET D7
#endif

#ifdef LOLIN
#define DEVICE_RESET 32
#endif

#ifdef USE_MMA8451
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#endif

#ifdef USE_MPU6050
#include "Accelerometer.h"
#endif

LightInstrument device;
bool deviceReady = false;

#ifdef USE_MMA8451
Adafruit_MMA8451 mma = Adafruit_MMA8451();
#endif

float lastX = 0, lastY = 0, lastZ = 0;
bool peakSearchX = false, peakSearchY = false, peakSearchZ = false;
const float threshold = 30.0; // Acceleration threshold for shake detection
unsigned long lastEventTime = 0;
const unsigned long cooldown = 100; // ms between events to avoid double triggering

void processingTask(void* pvParameters) {
  for (;;) {
    #ifdef USE_MMA8451
    sensors_event_t event;
    mma.getEvent(&event);

    float x = abs(event.acceleration.x);
    float y = abs(event.acceleration.y);
    float z = abs(event.acceleration.z);
    #endif

    #ifdef USE_MPU6050
    AccelerationReading reading;
    readAccelerationValues(&reading);

    float x = reading.x;
    float y = reading.y;
    float z = reading.z;
    #endif

    bool triggered = false;
    unsigned long now = millis();

    if (now - lastEventTime > cooldown) {
      if (x > threshold) {
        if (x > lastX) {
          peakSearchX = true;
        } else if (peakSearchX) {
          triggered = true;
        }
      } else {
        peakSearchX = false;
      }

      if (y > threshold) {
        if (y > lastY) {
          peakSearchY = true;
        } else if (peakSearchY) {
          triggered = true;
        }
      } else {
        peakSearchY = false;
      }

      if (z > threshold) {
        if (z > lastZ) {
          peakSearchZ = true;
        } else if (peakSearchZ) {
          triggered = true;
        }
      } else {
        peakSearchZ = false;
      }

      if (triggered) {
        device.sendEvent("accel", 1);

        lastEventTime = now;
        peakSearchX = false;
        peakSearchY = false;
        peakSearchZ = false;
      }
    }

    lastX = x;
    lastY = y;
    lastZ = z;

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void setup() {
  device.begin(DEVICE_RESET);
  delay(2000);

  if (device.getDeviceName() == "") {
    while (device.getDeviceName() == "") {
      device.listenForDeviceConfig();
      delay(100);
    }
  }

  device.signalBootStart();

  #ifdef USE_MMA8451
  if (!mma.begin()) {
    while (1);
  }
  mma.setRange(MMA8451_RANGE_4_G);
  #endif

  #ifdef USE_MPU6050
  initAccelerometer();
  #endif
}

void loop() {
  device.update();

  if (!deviceReady) {
    if (!device.isRelayFound()) {
      return;
    }

    device.signalDeviceReady();
    xTaskCreatePinnedToCore(processingTask, "ProcessingTask", 4096, NULL, 1, NULL, 1);
    deviceReady = true;
  }

  vTaskDelay(pdMS_TO_TICKS(100));
}
