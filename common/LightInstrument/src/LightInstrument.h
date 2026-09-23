#ifndef LIGHT_INSTRUMENT_H
#define LIGHT_INSTRUMENT_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Preferences.h>
#include "Protocol.h"

class LightInstrument {
public:
  LightInstrument(const char* deviceType = "sensor");
  void begin(int resetPin = -1);
  void update();

  void sendEvent(const char* port, int32_t value);

  bool listenForDeviceConfig();
  String getDeviceName() const { return deviceName; }
  int getWifiConfig() const { return wifiChannel; }
  const uint8_t* getRelayAddress() const { return relayAddress; }
  bool isRelayFound() const { return relayFound; }

  void signalBootStart();
  void signalDeviceReady();

private:
  static void onDataRecvStatic(const uint8_t * mac, const uint8_t *incomingData, int len);
  void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);

  void sendDiscoveryResponse();
  void sendPong();

  bool initDeviceConfig();
  void saveDeviceName(String name);
  void saveWifiChannel(int channel);
  void handleMemoryReset(int resetPin);

  String deviceName;
  const char* deviceType;
  uint8_t relayAddress[6];
  int wifiChannel = 0;
  bool relayFound = false;
  bool pingReceived = false;

  static LightInstrument* instance;
};

#endif
