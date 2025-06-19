#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Pin Definitions
const uint8_t BUTTON_PINS[] = {2, 3, 4, 5};
const uint8_t TOTAL_BUTTONS = 4;

// LCD Configuration
const uint8_t LCD_ADDRESS = 0x27;
const uint8_t LCD_COLS = 16;
const uint8_t LCD_ROWS = 2;

// RTC Pins
const uint8_t RTC_CLK = 7;
const uint8_t RTC_DAT = 6;
const uint8_t RTC_RST = 8;

// Sensor Pins
const int SOIL_POWER_PIN = 11;
const int SOIL_READ_PIN = A2;
const int WATER_DETECTION_READ = A3;
const int WATER_DETECTION_POWER = 13;
const int WATER_SENSOR_PIN = 12;

// Pump Pins
const int PUMP_VALVE_PIN = 9;
const int PUMP_PIN = 10;

// Sensor Constants
const int DRY_VALUE = 300;
const int WET_VALUE = 880;
const int READ_DELAY = 200;
const int WATER_DETECT_THRESHOLD = 350;

// Pump Settings
const int PUMP_VALVE_TIMING = 2000;
const int PUMP_LOW_SETTING = 90;
const int PUMP_MID_SETTING = 150;
const int PUMP_HIGH_SETTING = 255;

// Timing Constants
const unsigned long MESSAGE_DISPLAY_DURATION = 3000;
const unsigned long DATE_AND_COUNTDOWN_DELAY = 6000;
const unsigned long INPUT_DEBOUNCE_DELAY = 25;
const unsigned long BOOT_ANIMATION_DELAY = 50;
const unsigned long BOOT_WAIT = 3500;
const unsigned long TRANSITION_DELAY = 2000;
const unsigned long EXIT_DELAY = 1000;
const unsigned long SENSOR_WARM_TIME = 200;
const unsigned long BLINK_INTERVAL = 500;
const unsigned long DEBOUNCE_DURATION = 100;

// Default Values
const unsigned long DEFAULT_WATER_INTERVAL_HOUR = 60;
const unsigned long WATER_INTERVAL_DELTA = 60;
const unsigned long DEFAULT_WATER_DURATION = 20000UL;
const float DEFAULT_WATER_CUPS_TARGET = 1.0;

#endif
