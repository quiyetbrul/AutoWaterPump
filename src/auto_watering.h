#ifndef AUTO_WATERING_H
#define AUTO_WATERING_H

#include "config.h"
#include "pump_control.h"
#include "rtc_clock.h"
#include "sensors.h"
#include "types.h"

class AutoWateringSystem {
private:
  SensorManager *sensors;
  PumpController *pump;
  RTCClock *rtc;

  // Auto watering settings
  bool autoModeEnabled;
  unsigned long wateringIntervalHours;
  float targetWaterCups;
  PumpSpeed autoWaterSpeed;

  // Timing
  unsigned long lastWateringTime;
  unsigned long nextWateringTime;

  // Status
  bool needsWatering;
  bool wateringInProgress;

  // Internal functions
  bool shouldWaterBasedOnTime();
  bool shouldWaterBasedOnMoisture();
  unsigned long calculateWateringDuration();
  void updateNextWateringTime();

public:
  AutoWateringSystem(SensorManager *sensorMgr, PumpController *pumpCtrl,
                     RTCClock *rtcClock);

  // Configuration
  void setWateringInterval(unsigned long hours);
  void setTargetWaterAmount(float cups);
  void setWateringSpeed(PumpSpeed speed);
  void enableAutoMode(bool enable);

  // Main operation
  void update(); // Call this in main loop
  void checkAndWater();
  void forceWatering();
  void stopWatering();

  // Status getters
  bool isAutoModeEnabled() const;
  bool isWateringNeeded() const;
  bool isCurrentlyWatering() const;
  unsigned long getTimeUntilNextWatering() const;
  unsigned long getLastWateringTime() const;

  // Settings getters
  unsigned long getWateringInterval() const;
  float getTargetWaterAmount() const;
  PumpSpeed getWateringSpeed() const;

  // Reset functions
  void resetWateringSchedule();
  void markAsWatered();
};

#endif
