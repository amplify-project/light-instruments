#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#ifdef USE_MPU6050

#include <Arduino.h>
#include <Wire.h>

typedef struct AccelerationReading {
  float x;
  float y;
  float z;
} AccelerationReading;

void initAccelerometer() {
  Wire.begin(SDA, SCL);                       // Initialize comunication
  Wire.beginTransmission(0x68);       // Start communication with MPU6050 // MPU=0x68
  Wire.write(0x6B);                   // Talk to the register 6B
  Wire.write(0x00);                   // Reset - place a 0 into the 6B register
  Wire.endTransmission(true);

  Wire.beginTransmission(0x68);
  Wire.write(0x1C);                  // ACCEL_CONFIG register
  Wire.write(0x08);                  // Set range to +-4g
  Wire.endTransmission(true);
}

bool readAccelerationValues(AccelerationReading *reading) {
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 6, true);

  int16_t rawX = Wire.read() << 8 | Wire.read();
  int16_t rawY = Wire.read() << 8 | Wire.read();
  int16_t rawZ = Wire.read() << 8 | Wire.read();

  float x = abs(rawX / 8192.0 * 9.80665);
  float y = abs(rawY / 8192.0 * 9.80665);
  float z = abs(rawZ / 8192.0 * 9.80665);

  reading->x = x;
  reading->y = y;
  reading->z = z;

  return true;
}

#endif
#endif
