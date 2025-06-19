#ifndef MENU_SYSTEM_H
#define MENU_SYSTEM_H

#include "button_input.h"
#include "config.h"
#include "lcd_display.h"
#include "types.h"

class MenuSystem {
private:
  LCDDisplay *display;
  ButtonInput *buttons;

  MenuPage currentPage;
  int currentSelection;
  int maxSelections;
  bool inSubMenu;
  unsigned long lastInteraction;

  // Menu content arrays
  static const char *mainMenuItems[];
  static const char *settingsMenuItems[];
  static const char *autoWateringMenuItems[];
  static const char *calibrationMenuItems[];

  // Menu navigation helpers
  void navigateUp();
  void navigateDown();
  void selectItem();
  void goBack();
  void updateSelection();
  void resetToMainMenu();

  // Menu display functions
  void displayMainMenu();
  void displaySettingsMenu();
  void displayAutoWateringMenu();
  void displayManualWateringMenu();
  void displayCalibrationMenu();
  void displayDebugMenu();

  // Input timeout handling
  bool isInputTimeout();
  void resetTimeout();

public:
  MenuSystem();

  // Initialization
  void init(LCDDisplay *lcd, ButtonInput *buttonInput);

  // Main menu operations
  void showMainMenu();
  void displayCurrentMenu();
  void handleButtonPress(int buttonIndex);
  void update(); // Call in main loop for timeout handling

  // Navigation
  MenuPage getCurrentPage() const;
  int getCurrentSelection() const;
  bool isInSubMenu() const;

  // Menu actions (these would trigger actual operations)
  void executeMenuAction();

  // Timeout settings
  void setMenuTimeout(unsigned long timeoutMs);
  static const unsigned long DEFAULT_MENU_TIMEOUT = 30000; // 30 seconds
};

// Menu item definitions
const char *MenuSystem::mainMenuItems[] = {
    "Auto Watering", "Manual Water", "Settings", "Calibration", "Debug Info"};

const char *MenuSystem::settingsMenuItems[] = {"Water Interval", "Water Amount",
                                               "Pump Speed", "Auto Mode",
                                               "Reset Settings"};

const char *MenuSystem::autoWateringMenuItems[] = {
    "Enable/Disable", "Next Watering", "Last Watered", "Force Water", "Back"};

const char *MenuSystem::calibrationMenuItems[] = {
    "Start Calibration", "Reset Calibration", "View Current", "Back"};

#endif
