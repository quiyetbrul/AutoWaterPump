#ifndef SENSORS_H
#define SENSORS_H

#include "config.h"
#include "types.h"

class SensorManager {
private:
  int lastMoistureReading;
  bool lastWaterDetection;
  unsigned long lastReadTime;

  // Internal helper functions
  int readSoilMoisture();
  bool readWaterDetection();

public:
  SensorManager();

  // Initialization
  void init();

  // Reading functions
  void updateReadings();
  int getMoistureLevel();
  int getMoisturePercentage();
  bool isWaterDetected();
  bool isSoilDry();
  bool isSoilWet();

  // Status functions
  SensorStatus getSoilStatus();
  SensorStatus getWaterStatus();
  bool sensorsHealthy();

  // Calibration helpers
  void startCalibrationMode();
  void endCalibrationMode();
  int getRawMoistureReading();

  // Utility functions
  void powerSensors(bool enable);
  unsigned long getLastReadTime() const;
};

#endif
