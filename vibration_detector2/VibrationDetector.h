#ifndef VIBRATION_DETECTOR_H
#define VIBRATION_DETECTOR_H

#include <Arduino.h>
#include <esp_adc_cal.h>

class VibrationDetector {
private:
  esp_adc_cal_characteristics_t adcChars;

  float currentPeak = 0.0f;
  uint32_t lockoutUntil = 0;
  bool detectedInWindow = false;

  // Configuration
  const float spikeThreshold = 300.0f;    // mV to trigger detection
  const uint32_t lockoutDuration = 20;   // ms to ignore bounces
  const float peakDecayRate = 0.95f;      // Decay for analog output

public:
  VibrationDetector() {}

  void begin(uint8_t pin);
  float update(uint16_t rawAdc);

  bool isVibrating();

  float getPeak() const;
  uint16_t getVibrationAmount() const;
};

#endif
