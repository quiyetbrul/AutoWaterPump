#include <Arduino.h>

#include "auto_watering.h"
#include "config.h"
#include "lcd_display.h"
#include "menu_system.h"
#include "pump_control.h"
#include "sensors.h"
#include "types.h"

#include <string>

// Global objects
LCDDisplay display;
SensorManager sensors;
PumpController pump;
RTCClock rtcClock;
ButtonInput buttons;
MenuSystem menu;
SettingsManager settings;
AutoWateringSystem autoWatering(&sensors, &pump, &rtcClock);

// Global state
SystemState currentState = IDLE;
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 100; // Update every 100ms

void displayMainScreen();
void displayAutoWateringInfo();
void displayManualWateringControls();
void displaySettingsMenu();
void displayCalibrationMenu();
void displayDebugInfo();
void handleUserInput();
void updateDisplay();
void handleStateChanges();

void setup() {
  Serial.begin(9600);

  // Initialize all modules
  display.init();
  sensors.init();
  pump.init();
  rtcClock.init();
  buttons.init();
  settings.loadFromEEPROM();

  // Configure auto watering with saved settings
  autoWatering.setWateringInterval(settings.getWateringInterval());
  autoWatering.setTargetWaterAmount(settings.getTargetWaterAmount());
  autoWatering.setWateringSpeed(settings.getPumpSpeed());
  autoWatering.enableAutoMode(settings.isAutoModeEnabled());

  // Show boot animation
  display.displayBootAnimation();
  delay(BOOT_WAIT);

  // Initialize menu system
  menu.init(&display, &buttons);
  menu.showMainMenu();
}

void loop() {
  unsigned long currentTime = millis();

  // Update at fixed intervals
  if (currentTime - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = currentTime;

    // Update all modules
    buttons.update();
    sensors.updateReadings();
    pump.update();
    autoWatering.update();

    // Handle user input
    handleUserInput();

    // Update display based on current state
    updateDisplay();

    // Handle system state changes
    handleStateChanges();
  }
}

void handleUserInput() {
  ButtonState buttonStates[TOTAL_BUTTONS];
  buttons.getButtonStates(buttonStates);

  // Pass button events to menu system
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    if (buttonStates[i] == BUTTON_PRESSED) {
      menu.handleButtonPress(i);
    }
  }
}

void updateDisplay() {
  MenuPage currentPage = menu.getCurrentPage();

  switch (currentPage) {
  case MAIN_MENU:
    displayMainScreen();
    break;

  case AUTO_WATERING_MENU:
    displayAutoWateringInfo();
    break;

  case MANUAL_WATERING_MENU:
    displayManualWateringControls();
    break;

  case SETTINGS_MENU:
    displaySettingsMenu();
    break;

  case CALIBRATION_MENU:
    displayCalibrationMenu();
    break;

  case DEBUG_MENU:
    displayDebugInfo();
    break;
  }
}

void displayMainScreen() {
  // Get current time and sensor data
  DateTime currentTime = rtcClock.getCurrentDateTime();
  int moisturePercent = sensors.getMoisturePercentage();
  bool waterDetected = sensors.isWaterDetected();

  // Show time and basic status
  display.clear();
  display.setCursor(0, 0);
  display.print(String(currentTime.hour) + ":" + String(currentTime.minute) +
                " " + String(currentTime.day) + "/" +
                String(currentTime.month));

  display.setCursor(0, 1);
  if (autoWatering.isCurrentlyWatering()) {
    display.print("Watering...");
  } else {
    display.print("Soil: " + std::to_string(moisturePercent) + "%");
    if (!waterDetected) {
      display.print(" NO H2O");
    }
  }
}

void displayAutoWateringInfo() {
  display.clear();
  display.setCursor(0, 0);
  display.print("Auto: " + std::to_string(autoWatering.isAutoModeEnabled()
                                              ? "ON"
                                              : "OFF"));

  display.setCursor(0, 1);
  if (autoWatering.isAutoModeEnabled()) {
    unsigned long timeUntil = autoWatering.getTimeUntilNextWatering();
    display.print("Next: " + std::to_string(timeUntil / 3600000) + "h");
  } else {
    display.print("Press * to enable");
  }
}

void displayManualWateringControls() {
  display.clear();
  display.setCursor(0, 0);
  display.print("Manual Watering");

  display.setCursor(0, 1);
  if (pump.isPumpRunning()) {
    display.print("Running... #=Stop");
  } else {
    display.print("*=Start #=Menu");
  }
}

void displaySettingsMenu() {
  // This would be handled by the MenuSystem class
  menu.displayCurrentMenu();
}

void displayCalibrationMenu() {
  // This would be handled by the MenuSystem class
  menu.displayCurrentMenu();
}

void displayDebugInfo() {
  display.clear();
  display.setCursor(0, 0);
  display.print("Raw: " + std::to_string(sensors.getMoistureLevel()));

  display.setCursor(0, 1);
  display.print(
      "Pump: " + std::to_string(pump.isPumpRunning() ? "ON" : "OFF") +
      " Val: " + std::to_string(pump.isValveOpen() ? "OPEN" : "CLOSED"));
}

void handleStateChanges() {
  // Handle transitions between system states
  switch (currentState) {
  case IDLE:
    if (autoWatering.isCurrentlyWatering()) {
      currentState = WATERING;
    }
    break;

  case WATERING:
    if (!autoWatering.isCurrentlyWatering() && !pump.isPumpRunning()) {
      currentState = IDLE;
    }
    break;

  case ERROR_STATE:
    // Check if errors are resolved
    if (sensors.sensorsHealthy() && pump.isSafeToOperate()) {
      currentState = IDLE;
    }
    break;

  case CALIBRATING:
    // This would be managed by the calibration module
    break;
  }

  // Handle error conditions
  if (!sensors.sensorsHealthy() || !pump.isSafeToOperate()) {
    if (currentState != ERROR_STATE) {
      currentState = ERROR_STATE;
      pump.emergencyStop();
      display.displayError("System Error!");
    }
  }
}
