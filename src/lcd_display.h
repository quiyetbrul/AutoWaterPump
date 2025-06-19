#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include "config.h"
#include "types.h"
#include <LiquidCrystal_I2C.h>

class LCDDisplay {
private:
  LiquidCrystal_I2C *lcd;
  unsigned long lastBlinkTime;
  bool blinkState;

public:
  LCDDisplay();
  ~LCDDisplay();

  // Basic display functions
  void init();
  void clear();
  void setCursor(int col, int row);
  void print(const String &text);
  void print(int value);
  void print(float value, int decimals = 1);

  // Specialized display functions
  void displayBootAnimation();
  void displayMainMenu();
  void displayDateTime(int hour, int minute, int day, int month, int year);
  void displayCountdown(unsigned long remainingTime);
  void displaySensorStatus(int moisture, bool waterDetected);
  void displayWateringStatus(bool isWatering, unsigned long duration);
  void displayCalibrationInfo(float cups, unsigned long duration);
  void displaySettings(WateringMode mode, unsigned long interval,
                       float targetCups);
  void displayMessage(const String &line1, const String &line2 = "",
                      unsigned long duration = 0);

  // Menu navigation helpers
  void highlightMenuItem(int selectedItem, const String items[], int itemCount);
  void displayProgressBar(int progress, int total);
  void blinkCursor(int col, int row);

  // Error handling
  void displayError(const String &errorMsg);
  void displayWarning(const String &warningMsg);
};

#endif
