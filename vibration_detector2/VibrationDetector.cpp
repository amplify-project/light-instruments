#include "VibrationDetector.h"

void VibrationDetector::begin(uint8_t pin) {
  analogReadResolution(12);
  analogSetPinAttenuation(pin, ADC_11db);

  esp_adc_cal_characterize(
    ADC_UNIT_1,
    ADC_ATTEN_DB_12,
    ADC_WIDTH_BIT_12,
    0,
    &adcChars
  );
}

float VibrationDetector::update(uint16_t rawAdc) {
  uint32_t voltageMv = esp_adc_cal_raw_to_voltage(rawAdc, &adcChars);
  float sample = static_cast<float>(voltageMv);

  // Dynamic baseline tracking (High-pass offset cancellation)
  rawDC = (alpha * sample) + ((1.0f - alpha) * rawDC);

  // Rectification (Extract AC variance magnitude)
  float acSignal = fabsf(sample - rawDC);

  // Low-pass envelope creation
  envelope = (beta * acSignal) + ((1.0f - beta) * envelope);

  // Adapt ambient noise floor down during quiet periods
  if (envelope < noiseFloor) {
    noiseFloor = (0.005f * envelope) + (0.995f * noiseFloor);
  }

  return envelope;
}

bool VibrationDetector::isVibrating() const {
  return envelope > (noiseFloor + sensitivityMargin);
}

float VibrationDetector::getEnvelope() const {
  return envelope;
}

float VibrationDetector::getNoiseFloor() const {
  return noiseFloor;
}
