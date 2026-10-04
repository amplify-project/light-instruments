#ifndef ANALOG_LIGHT_INSTRUMENT_H
#define ANALOG_LIGHT_INSTRUMENT_H

#include "LightInstrument.h"

class AnalogLightInstrument : public LightInstrument {
public:
  bool getIsAnalog() const { return isAnalog; };

protected:
  bool initDeviceConfig() override;
  bool parseReceivedSetting(String input) override;
  void printCurrentSettings() override;

private:
  void saveIsAnalog(bool analog);
  bool isAnalog;
};

#endif
