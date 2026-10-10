#include "VibrationDetector.h"

void VibrationDetector::begin(uint8_t pin) {
  analogReadResolution(12);
  analogSetPinAttenuation(pin, ADC_11db);

  esp_adc_cal_characterize(
    ADC_UNIT_1,
    ADC_ATTEN_DB_11,
    ADC_WIDTH_BIT_12,
    0,
    &adcChars
  );
}

float VibrationDetector::update(uint16_t rawAdc) {
  uint32_t voltageMv = esp_adc_cal_raw_to_voltage(rawAdc, &adcChars);
  float sample = static_cast<float>(voltageMv);
  uint32_t now = millis();

  // Decay the tracked peak for analog reporting
  currentPeak *= peakDecayRate;

  // Check for new spike if we are not in lockout
  if (now >= lockoutUntil) {
    if (sample > spikeThreshold) {
      detectedInWindow = true;
      lockoutUntil = now + lockoutDuration;

      // Update peak if this spike is higher than current decaying peak
      if (sample > currentPeak) {
        currentPeak = sample;
      }
    }
  }

  return currentPeak;
}

bool VibrationDetector::isVibrating() {
  bool detected = detectedInWindow;
  detectedInWindow = false; // Reset after reading
  return detected;
}

float VibrationDetector::getPeak() const {
  return currentPeak;
}

uint16_t VibrationDetector::getVibrationAmount() const {
  // Map 0-2500mV to 0-1023
  float val = (currentPeak / 2500.0f) * 1023.0f;

  if (val < 0.0f) val = 0.0f;
  if (val > 1023.0f) val = 1023.0f;

  return static_cast<uint16_t>(val);
}
