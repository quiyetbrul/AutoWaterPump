#ifndef PUMP_CONTROL_H
#define PUMP_CONTROL_H

#include "config.h"
#include "types.h"

class PumpController {
private:
  bool pumpActive;
  bool valveOpen;
  PumpSpeed currentSpeed;
  unsigned long pumpStartTime;
  unsigned long targetDuration;

public:
  PumpController();

  // Initialization
  void init();

  // Pump control
  void startPump(PumpSpeed speed, unsigned long duration = 0);
  void stopPump();
  void setPumpSpeed(PumpSpeed speed);

  // Valve control
  void openValve();
  void closeValve();

  // Status functions
  bool isPumpRunning() const;
  bool isValveOpen() const;
  PumpSpeed getCurrentSpeed() const;
  unsigned long getRunTime() const;
  unsigned long getRemainingTime() const;

  // Automatic management
  void update(); // Call in main loop to handle automatic shutoff

  // Watering sequences
  void startWateringSequence(PumpSpeed speed, unsigned long duration);
  void stopWateringSequence();

  // Safety functions
  void emergencyStop();
  bool isSafeToOperate();
};

#endif
