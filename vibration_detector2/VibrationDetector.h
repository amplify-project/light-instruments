#ifndef VIBRATION_DETECTOR_H
#define VIBRATION_DETECTOR_H

#include <Arduino.h>
#include <esp_adc_cal.h>

class VibrationDetector {
private:
  float rawDC = 2048.0f;
  float envelope = 0.0f;
  float noiseFloor = 50.0f;

  // Filter coefficients
  const float alpha = 0.02f;
  const float beta = 0.25f;
  const float sensitivityMargin = 150.0f;

  esp_adc_cal_characteristics_t adcChars;

public:
  VibrationDetector() {}

  void begin(uint8_t pin);
  float update(uint16_t rawAdc);

  bool isVibrating() const;

  float getEnvelope() const;
  float getNoiseFloor() const;
};

#endif
