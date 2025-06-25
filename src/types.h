#ifndef TYPES_H
#define TYPES_H

// Enums for better code organization
enum MenuPage {
  MAIN_MENU = 0,
  AUTO_WATERING_MENU = 1,
  MANUAL_WATERING_MENU = 2,
  SETTINGS_MENU = 3,
  CALIBRATION_MENU = 4,
  DEBUG_MENU = 5
};

enum WateringMode { AUTO_MODE, MANUAL_MODE, OFF_MODE };

enum PumpSpeed { PUMP_LOW = 0, PUMP_MID = 1, PUMP_HIGH = 2 };

enum SensorStatus { SENSOR_OK, SENSOR_ERROR, SENSOR_DRY, SENSOR_WET };

// Button states
enum ButtonState { BUTTON_RELEASED, BUTTON_PRESSED, BUTTON_HELD };

// System states
enum SystemState { IDLE, WATERING, CALIBRATING, ERROR_STATE };

#endif
