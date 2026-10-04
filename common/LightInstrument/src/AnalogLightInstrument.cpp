#include "AnalogLightInstrument.h"

void AnalogLightInstrument::saveIsAnalog(bool analog) {
  Preferences prefs;
  prefs.begin("system", false);
  prefs.putBool("analog", analog);
  prefs.end();

  isAnalog = analog;
}

void AnalogLightInstrument::printCurrentSettings() {
  Serial.printf("type=sensor,name=%s,channel=%d,analog=%s\n", getDeviceName(), getWifiConfig(), (getIsAnalog()) ? "true" : "false");
}

bool AnalogLightInstrument::parseReceivedSetting(String input) {
  if (input.startsWith("analog=")) {
    String newValue = input.substring(7);

    if (newValue == "true") {
      saveIsAnalog(true);
    } else if (newValue == "false") {
      saveIsAnalog(false);
    } else {
      Serial.println("ERR");
      return false;
    }

    Serial.println("OK");
    return true;
  }

  return LightInstrument::parseReceivedSetting(input);
}

bool AnalogLightInstrument::initDeviceConfig() {
  Preferences prefs;
  prefs.begin("system", true);
  isAnalog = prefs.getBool("analog", false);
  prefs.end();

  return LightInstrument::initDeviceConfig();
}
