/**
 * @file main.cpp
 * @brief Automatic Plant Watering System
 * @author Quiyet Brul
 * @date 2025
 * @version 2.0
 *
 * @details This Arduino-based automatic plant watering system provides:
 * - Smart scheduling with configurable intervals
 * - Real-time soil moisture monitoring
 * - Manual and automatic watering modes
 * - LCD display with intuitive menu system
 * - Water level detection and safety features
 * - Pump calibration for precise water dispensing
 *
 * @see README.md for detailed hardware setup and wiring diagram
 */

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <RtcDS1302.h>
#include <Wire.h>

// ========================================
// HARDWARE CONFIGURATION
// ========================================

/**
 * @brief LCD display object (16x2 characters, I2C interface)
 * @details Uses I2C address 0x27, common for most I2C LCD modules
 */
LiquidCrystal_I2C lcd(0x27, 16, 2);

/**
 * @name RTC (Real-Time Clock) Configuration
 * @brief DS1302 RTC module pin assignments and initialization
 * @{
 */
const byte CLK = 7; ///< Clock pin for DS1302 RTC
const byte DAT = 6; ///< Data pin for DS1302 RTC
const byte RST = 8; ///< Reset pin for DS1302 RTC
ThreeWire rtcWire(DAT, CLK, RST);
RtcDS1302<ThreeWire> rtc(rtcWire);
/** @} */

/**
 * @name Button Configuration
 * @brief Navigation buttons for menu system
 * @{
 */
const byte buttonPins[] = {2, 3, 4, 5}; ///< Button pins: [-, +, M, A]
const byte totalButtons = sizeof(buttonPins) / sizeof(buttonPins[0]);
/** @} */

/**
 * @name Sensor Pin Configuration
 * @brief Analog and digital pins for moisture and water sensors
 * @{
 */
const unsigned char pinSoilPower = 11; ///< Power pin for soil moisture sensor
const unsigned char pinSoilRead = A2;  ///< Analog read pin for soil moisture
const unsigned char waterDetectionRead =
    A3; ///< Analog read pin for water detection
const unsigned char waterDetectionPower =
    13; ///< Power pin for water detection sensor
/** @} */

/**
 * @name Sensor Thresholds and Calibration
 * @brief Calibration values and thresholds for sensor readings
 * @{
 */
const unsigned char waterSensorPin = 12; ///< Digital pin for water level sensor
const unsigned int DRY_VALUE = 300; ///< Raw ADC value for completely dry soil
const unsigned int WET_VALUE = 880; ///< Raw ADC value for completely wet soil
const unsigned int waterDetectThreshold =
    350; ///< Threshold for water detection sensor
unsigned long lastRawMoistureValue = 0; ///< Last raw moisture reading cache
/** @} */

/**
 * @name Pump and Valve Configuration
 * @brief Water pump control pins and timing parameters
 * @{
 */
const unsigned char pumpValvePin = 9; ///< Digital pin controlling water valve
const unsigned char pumpPin = 10;     ///< PWM pin controlling pump speed
const unsigned int pumpValveTiming = 2000; ///< Valve operation delay (ms)
const unsigned char pumpHighSetting = 255; ///< Full speed PWM value for pump
/** @} */

/**
 * @name Watering System Variables
 * @brief Runtime variables for automatic watering control
 * @{
 */
unsigned int waterInterval = 0;        ///< Time between waterings (minutes)
unsigned int waterIntervalHour = 60;   ///< Default watering interval (minutes)
unsigned int waterIntervalDelta = 60;  ///< Increment step for interval setting
unsigned long waterDuration = 20000UL; ///< Duration of watering cycle (ms)
float moistureLevel = 0.0;             ///< Current soil moisture percentage
unsigned long oneCupCalibrated = 0; ///< Calibrated time for 1 cup of water (ms)
unsigned long autoWaterDurationMillis =
    0; ///< Calculated watering duration for auto mode
/** @} */

/**
 * @name Timing Constants
 * @brief Various timing delays and intervals for UI and system operation
 * @{
 */
const unsigned int messageDisplayDuration =
    3000; ///< Main menu message display time
const unsigned int dateAndCountDownDelay =
    6000;                                    ///< Clock display toggle interval
const unsigned char inputDebounceDelay = 25; ///< Button debounce delay
const unsigned char bootAnimationDelay = 50; ///< Boot animation character delay
const unsigned int bootWait = 3500;          ///< Initial boot wait time
const unsigned int transitionDelay = 2000;   ///< Menu transition delay
const unsigned int exitDelay = 1000;         ///< Exit message display time
const unsigned char sensorWarmTime = 200;    ///< Sensor stabilization time
const unsigned int blinkInterval = 500;      ///< Clock colon blink interval
/** @} */

/**
 * @name Button State Variables
 * @brief Variables for button debouncing and state tracking
 * @{
 */
unsigned long lastTimeButtonStateChanged = 0; ///< Last button state change time
unsigned char debounceDuration = 100;         ///< Button debounce duration
unsigned char lastButtonState;                ///< Previous button state
/** @} */

/**
 * @name System State Variables
 * @brief Global state variables for system operation modes
 * @{
 */
bool isAutoModeEnabled = false; ///< Flag indicating if auto watering is active
bool showInstructions = false;  ///< Flag to show/hide instruction messages
bool displayDateMessage = true; ///< Toggle between date and countdown display
unsigned char currentMenu = 0;  ///< Current active menu (0 = main menu)
unsigned char messageIndex = 0; ///< Index for cycling main menu messages
unsigned long lastMessageSwitch = 0; ///< Last time main menu message switched
static bool showColon = true;        ///< Clock colon visibility toggle
unsigned long lastBlink = 0;         ///< Last colon blink time
unsigned long autoTimer = 0;         ///< Timer for automatic watering intervals
/** @} */

/**
 * @brief Main menu message array
 * @details Contains the rotating messages displayed on the main menu
 */
String messagesHomeScreen[] = {
    "(-) Show Clock  ",
    "(+) Settings   ",
    "(M) Manual Mode ",
    "(A) Auto Mode   ",
};
const unsigned char totalMessages =
    sizeof(messagesHomeScreen) / sizeof(messagesHomeScreen[0]);

// ========================================
// CORE SYSTEM
// ========================================
void displayStartup();
void loop();
void setup();

// ========================================
// MAIN MENU & NAVIGATION
// ========================================
void showMessageCycle();
void checkButtons();
void handleMenu(unsigned char menu);
bool isButtonPressed(unsigned char pin);

// ========================================
// PRIMARY FEATURE
// ========================================
void showClock();
void showMessageCycleClock();
void manualWatering();
void autoWatering();
void waterPlant();
void autoWateringCheck();

// ========================================
// SETTINGS & CONFIGURATION
// ========================================
void settingsMenu();
void setDateTime();
void waterCalibrationTest();
void disableMessages();

// ========================================
// SENSOR & HARDWARE
// ========================================
float readSoilMoisture();
unsigned char calculateMoisture(unsigned int raw);
bool isWaterDetected();
bool isPlantOkayToWater();

// ========================================
// DISPLAY & UI UTILITY
// ========================================
void printMessage(int x, int y, const String &message);
void printAnimation(String message);
void printExitCurrentMenu();
void printInstructions();
void formatTime(int &hour, bool &isPM);

// ========================================
// STRING FORMATTING
// ========================================
String getMoistureValue();
String getNextFeed(unsigned long totalSecondsRemaining, unsigned long hoursPart,
                   unsigned long minutesPart);
String getTime(const RtcDateTime now);
String getDate(const RtcDateTime now);

/**
 * @brief Initializes all hardware and peripherals
 * @details Performs system initialization including:
 * - LCD display setup and welcome message
 * - Button pins configuration as INPUT_PULLUP
 * - Sensor and pump pin configuration
 * - RTC initialization
 * - Initial sensor reading
 * - Boot animation display
 */
void setup() {
  lcd.init();
  lcd.backlight();
  printMessage(2, 0, "Created by:");
  printMessage(2, 1, "Quiyet Brul");
  delay(2000);

  for (unsigned char i = 0; i < totalButtons; ++i) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  pinMode(pinSoilPower, OUTPUT);
  pinMode(pinSoilRead, INPUT_PULLUP);
  pinMode(waterSensorPin, INPUT_PULLUP);
  pinMode(waterDetectionPower, OUTPUT);
  pinMode(pumpValvePin, OUTPUT);

  digitalWrite(waterDetectionPower, LOW);
  digitalWrite(pumpValvePin, LOW);
  readSoilMoisture();
  rtc.Begin();

  displayStartup();
  lastMessageSwitch = millis();
}

/**
 * @brief Main program loop - handles water level checking and menu navigation
 * @details Core system loop that:
 * - Checks for low water level and displays warning
 * - Shows main menu cycling when no menu is active
 * - Processes button input for menu navigation
 * - Handles menu transitions and resets menu state
 */
void loop() {
  if (!isWaterDetected()) {
    lcd.clear();
    printMessage(0, 0, "Water Lvl Low!");
    printMessage(0, 1, "Please add water");
    delay(5000);
  } else if (currentMenu == 0) {
    showMessageCycle();
    checkButtons();
  } else {
    handleMenu(currentMenu);
    currentMenu = 0;
  }
}

/**
 * @brief Displays the startup animation and welcome screen
 * @details Shows "Water Pump Menu" title followed by animated loading text
 */
void displayStartup() {
  printMessage(0, 0, "Water Pump Menu");
  printAnimation("    Loading...");
  // delay(bootWait);
}

/**
 * @brief Cycles through main menu messages on the LCD display
 * @details Automatically rotates through the main menu options every N seconds
 * Updates the message index and refreshes the display when the timer expires
 */
void showMessageCycle() {
  if (millis() - lastMessageSwitch >= messageDisplayDuration) {
    lcd.noBlink();
    lcd.clear();
    printMessage(4, 0, "MainMenu");
    printMessage(0, 1, messagesHomeScreen[messageIndex]);

    messageIndex = (messageIndex + 1) % totalMessages;
    lastMessageSwitch = millis();
  }
}

/**
 * @brief Checks all navigation buttons and sets menu selection
 * @details Implements throttled button checking (every 10ms) to reduce CPU
 * usage Maps button presses to menu numbers: button 0→menu 1, button 1→menu 2,
 * etc.
 */
void checkButtons() {
  static unsigned long lastCheck = 0;
  unsigned long now = millis();

  // Throttle button checking to reduce CPU usage
  if (now - lastCheck < 10)
    return; // Check every 10ms
  lastCheck = now;

  for (unsigned char i = 0; i < totalButtons; ++i) {
    if (isButtonPressed(buttonPins[i])) {
      currentMenu = i + 1;
      break;
    }
  }
}

/**
 * @brief Routes to appropriate menu function based on selection
 * @param menu Menu number to activate (1=Clock, 2=Settings, 3=Manual, 4=Auto)
 * @details Clears display, calls the selected menu function, then resets to
 * main menu
 */
void handleMenu(unsigned char menu) {
  lcd.clear();

  switch (menu) {
  case 1:
    showClock();
    break;
  case 2:
    settingsMenu();
    break;
  case 3:
    manualWatering();
    break;
  case 4:
    autoWatering();
    break;
  default:
    lcd.print("Unknown Option");
  }

  delay(100);
  lcd.clear();
}

/**
 * @brief Debounced button press detection
 * @param pin Arduino pin number to check
 * @return true if button was pressed (with debouncing), false otherwise
 * @details Implements software debouncing with 100ms duration to prevent
 * false triggers from mechanical button bounce
 */
bool isButtonPressed(unsigned char pin) {
  if (millis() - lastTimeButtonStateChanged >= debounceDuration) {
    byte buttonState = digitalRead(pin);
    if (buttonState != lastButtonState) {
      lastTimeButtonStateChanged = millis();
      lastButtonState = buttonState;
      if (buttonState == LOW) {
        return true;
      }
    }
  }
  return false;
}

/**
 * @brief Interactive clock display with moisture checking functionality
 * @details Shows cycling time/date display with options to check moisture level
 * or toggle auto mode. Provides real-time clock view with additional features:
 * - Cycles between time and date every few seconds
 * - Button 2: Measure and display current soil moisture
 * - Button 3: Exit menu or toggle auto watering mode
 * - Continuous auto watering check when enabled
 */
void showClock() {
  if (showInstructions) {
    printInstructions();
  }

  displayDateMessage = true;
  lastMessageSwitch = millis();
  lcd.clear();

  while (true) {
    showMessageCycleClock();
    autoWateringCheck();

    if (isButtonPressed(buttonPins[2])) {
      lcd.clear();
      printMessage(0, 0, "Moisture Lvl:");
      printAnimation("  Measuring...  ");
      readSoilMoisture();
      printMessage(0, 1, getMoistureValue());
      delay(messageDisplayDuration);
      lcd.clear();
      printMessage(0, 0, "      Done     ");
      lcd.clear();
    }
    if (isButtonPressed(buttonPins[3])) {
      if (!isAutoModeEnabled) {
        printExitCurrentMenu();
        return;
      }
      lcd.clear();
      printMessage(0, 0, "  [Auto Mode]");
      delay(500);
      printMessage(0, 1, "  Disabled :(");
      isAutoModeEnabled = false;
      delay(2000);
    }
    delay(inputDebounceDelay);
  }
}

/**
 * @brief Clock display cycle with time/date alternation and countdown
 * @details Handles the clock screen display logic including colon blinking,
 * time/date cycling, and countdown timer when auto mode is enabled
 */
void showMessageCycleClock() {
  unsigned long nowMs = millis();

  if (nowMs - lastBlink >= blinkInterval) {
    showColon = !showColon;
    lastBlink = nowMs;
  }

  RtcDateTime now = rtc.GetDateTime();
  printMessage(0, 0, getTime(now));
  printMessage(10, 0, getMoistureValue());

  if (!isAutoModeEnabled) {
    printMessage(0, 1, getDate(now));
    return;
  }

  if (millis() - lastMessageSwitch >= dateAndCountDownDelay) {
    displayDateMessage = !displayDateMessage;
    lastMessageSwitch = nowMs;
    printMessage(0, 1, "                ");
  }

  if (displayDateMessage) {
    printMessage(0, 1, getDate(now));
    return;
  }

  unsigned long elaspedTime = nowMs - autoTimer;
  unsigned long remainingTime = 0;
  unsigned long intervalMilli = waterInterval * 1000;

  if (elaspedTime < intervalMilli) {
    remainingTime = intervalMilli - elaspedTime;
  }

  unsigned long totalSecondsRemaining = remainingTime / 1000;
  unsigned long hoursPart = totalSecondsRemaining / 3600UL;
  unsigned long minutesPart = (totalSecondsRemaining % 3600UL) / 60UL;
  printMessage(0, 1, "Feeds in: ");
  printMessage(10, 1,
               getNextFeed(totalSecondsRemaining, hoursPart, minutesPart));
}

/**
 * @brief Manual watering mode with interactive pump control
 * @details Provides manual control interface for pump operation with real-time
 * moisture monitoring. Features include:
 * - Continuous moisture level display
 * - Button 2: Activate pump manually
 * - Button 3: Exit manual mode
 * - Real-time feedback during pump operation
 * - Safety delays to prevent pump damage
 */
void manualWatering() {
  if (showInstructions) {
    printInstructions();
  }

  delay(500);

  while (isPlantOkayToWater()) {
    lcd.clear();
    printMessage(0, 0, " Manual Water ");
    printMessage(0, 1, "(M)Hold (A):Esc ");

    bool currentlyWatering = false;

    while (true) {
      delay(inputDebounceDelay);
      bool isMHeld = (digitalRead(buttonPins[2]) == LOW);

      // Handle pump state changes
      if (isMHeld != currentlyWatering) {
        currentlyWatering = isMHeld;

        // Control pump based on current state
        if (currentlyWatering && isPlantOkayToWater()) {
          printMessage(0, 1, "  Watering...   ");
          digitalWrite(pumpValvePin, HIGH);
          analogWrite(pumpPin, pumpHighSetting);
        } else {
          printMessage(0, 1, "(M)Hold (A):Esc ");
          analogWrite(pumpPin, 0);
          digitalWrite(pumpValvePin, LOW);
        }
      }

      // Handle exit button
      if (isButtonPressed(buttonPins[3])) {
        // Stop pump if currently watering
        if (currentlyWatering) {
          analogWrite(pumpPin, 0);
          digitalWrite(pumpValvePin, LOW);
        }
        printExitCurrentMenu();
        return;
      }

      delay(50);
    }
  }
}

/**
 * @brief Automatic watering mode with smart scheduling
 * @details Automatic watering system with configurable parameters:
 * - Adjustable water amount (cups) using up/down buttons
 * - Time-based scheduling with countdown display
 * - Moisture threshold monitoring
 * - Manual override capability
 * - Exit option while maintaining auto mode state
 * Combines scheduled watering with moisture-based triggers for optimal plant
 * care
 */
void autoWatering() {
  if (showInstructions) {
    printInstructions();
  }

  float targetCups = 1.0;
  float stepp = 0.5;

  if (oneCupCalibrated <= 0) {
    lcd.clear();
    printMessage(0, 0, "Calibration");
    printMessage(0, 1, "Needed...");
    delay(1500);
    waterCalibrationTest();
    return;
  }

  delay(500);

  enum SettingStep { SET_VALUE, SET_FREQUENCY, DONE };
  SettingStep step = SET_VALUE;

  while (step != DONE) {
    lcd.clear();

    if (step == SET_VALUE) {
      printMessage(0, 0, "How much water?");
      printMessage(0, 1, "Cups: ");
      lcd.print(targetCups, 1);

      while (true) {
        // Handle increment/decrement
        int direction = 0;
        if (isButtonPressed(buttonPins[0]))
          direction = -1;
        if (isButtonPressed(buttonPins[1]))
          direction = 1;

        if (direction != 0) {
          while (digitalRead(buttonPins[abs(direction)]) == LOW)
            ;
          targetCups = constrain(targetCups + (direction * stepp), 0.5, 10.0);
          break;
        }

        if (isButtonPressed(buttonPins[2])) {
          autoWaterDurationMillis =
              (unsigned long)(targetCups * oneCupCalibrated);
          waterDuration = autoWaterDurationMillis;
          step = SET_FREQUENCY;
          break;
        }

        if (isButtonPressed(buttonPins[3])) {
          printExitCurrentMenu();
          return;
        }
      }
      delay(50);
    }

    if (step == SET_FREQUENCY) {
      printMessage(0, 0, "How frequent?");
      printMessage(0, 1, "Minutes: " + waterIntervalHour / 60);

      while (true) {
        // Handle increment/decrement
        int direction = 0;
        if (isButtonPressed(buttonPins[0]))
          direction = -1;
        if (isButtonPressed(buttonPins[1]))
          direction = 1;

        if (direction != 0) {
          unsigned int newInterval =
              waterIntervalHour + (direction * waterIntervalDelta);
          waterIntervalHour = constrain(newInterval, waterIntervalDelta, 1440);
          break;
        }

        if (isButtonPressed(buttonPins[2])) {
          waterInterval = waterIntervalHour;
          step = DONE;
          break;
        }

        if (isButtonPressed(buttonPins[3])) {
          printExitCurrentMenu();
          return;
        }
      }
      delay(100);
    }
  }

  isAutoModeEnabled = true;
  autoTimer = 0;
  lcd.clear();
  printMessage(0, 0, "  [Auto Mode]");
  delay(500);
  printMessage(0, 1, "  Enabled :)");
  delay(2000);
  autoTimer = millis();

  showClock();
}

/**
 * @brief Executes a complete watering cycle
 * @details Performs the physical watering operation with safety checks:
 * - Verifies plant is safe to water before starting
 * - Opens valve, runs pump for configured duration
 * - Closes valve and provides user feedback
 * - Uses proper timing delays to protect pump hardware
 */
void waterPlant() {
  if (isPlantOkayToWater()) {
    lcd.clear();
    printMessage(0, 0, "Watering plant..");
    digitalWrite(pumpValvePin, HIGH);
    delay(pumpValveTiming);
    analogWrite(pumpPin, pumpHighSetting);
    delay(waterDuration);
    analogWrite(pumpPin, 0);
    delay(pumpValveTiming);
    digitalWrite(pumpValvePin, LOW);
    lcd.clear();
    printMessage(0, 0, "Done!");
    delay(exitDelay);
    lcd.clear();
  }
}

/**
 * @brief Automatic watering timer check and execution
 * @details Monitors the automatic watering schedule when auto mode is enabled
 * Triggers watering when the configured interval has elapsed since last
 * watering Uses waterInterval (in seconds) to determine watering frequency
 */
void autoWateringCheck() {
  if (isAutoModeEnabled && (millis() - autoTimer >= waterInterval * 1000)) {
    waterPlant();
    autoTimer = millis();
  }
}

/**
 * @brief Interactive settings configuration menu
 * @details Provides user interface for adjusting system parameters:
 * - Moisture threshold adjustment (up/down buttons)
 * - Real-time threshold display with percentage
 * - Persistent settings storage during session
 * - Exit option to return to main menu
 * Allows fine-tuning of automatic watering sensitivity
 */
void settingsMenu() {
  if (showInstructions) {
    printInstructions();
  }

  const unsigned char totalSettings = 3;
  unsigned char selected = 0;
  unsigned char lastSelected = 255; // Force initial display

  String options[totalSettings] = {"1.Set Time/Date", "2.Calibrate Test",
                                   "3.Disable Msgs"};

  while (true) {
    // Only update display when selection changes
    if (selected != lastSelected) {
      lcd.clear();
      printMessage(0, 0, "Select Option:");
      printMessage(0, 1, options[selected]);
      lastSelected = selected;
    }

    delay(inputDebounceDelay);

    // Check all buttons and handle appropriately
    for (unsigned char i = 0; i < totalButtons; i++) {
      if (isButtonPressed(buttonPins[i])) {
        while (digitalRead(buttonPins[i]) == LOW)
          ; // Wait for release

        switch (i) {
        case 0: // Previous option
          selected = (selected == 0) ? totalSettings - 1 : selected - 1;
          break;
        case 1: // Next option
          selected = (selected + 1) % totalSettings;
          break;
        case 2: // Select/Confirm
          switch (selected) {
          case 0:
            setDateTime();
            break;
          case 1:
            waterCalibrationTest();
            break;
          case 2:
            disableMessages();
            break;
          }
          return;
        case 3: // Exit
          printExitCurrentMenu();
          return;
        }
        break; // Exit the for loop once a button is handled
      }
    }

    delay(50);
  }
}

/**
 * @brief Sets the date and time on the RTC module
 * @details Interactive interface for setting year, month, day, hour, and
 * minute Uses increment/decrement buttons to adjust values and saves to RTC
 */
void setDateTime() {
  int year = 2025;
  int month = 1;
  int day = 1;
  int hour = 12;
  int minute = 0;

  enum Step { SET_YEAR, SET_MONTH, SET_DAY, SET_HOUR, SET_MINUTE, DONE };
  Step step = SET_YEAR;
  Step lastStep = DONE; // Force initial display

  while (step != DONE) {
    // Only update display when step changes
    if (step != lastStep) {
      lcd.clear();
      char buffer[17]; // 16 chars + null terminator

      switch (step) {
      case SET_YEAR:
        sprintf(buffer, "Set Year: %d", year);
        break;
      case SET_MONTH:
        sprintf(buffer, "Set Month: %d", month);
        break;
      case SET_DAY:
        sprintf(buffer, "Set Day: %d", day);
        break;
      case SET_HOUR:
        sprintf(buffer, "Set Hour: %d", hour);
        break;
      case SET_MINUTE:
        sprintf(buffer, "Set Minute: %d", minute);
        break;
      default:
        break;
      }

      printMessage(0, 0, buffer);
      printMessage(0, 1, "(-)(+)(M)Next");
      lastStep = step;
    }

    bool buttonHandled = false;

    // finished setting date and time
    if (isButtonPressed(buttonPins[3])) {
      printExitCurrentMenu();
      return;
    }

    // finish setting current step
    if (isButtonPressed(buttonPins[2])) {
      step = static_cast<Step>(step + 1);
      buttonHandled = true;
    }

    // Handle increment/decrement buttons
    int direction = 0;
    if (isButtonPressed(buttonPins[0]))
      direction = -1; // Decrement
    if (isButtonPressed(buttonPins[1]))
      direction = 1; // Increment

    if (direction != 0) {
      switch (step) {
      case SET_YEAR:
        year = constrain(year + direction, 2000, 2099);
        break;
      case SET_MONTH:
        month = constrain(month + direction, 1, 12);
        break;
      case SET_DAY:
        day = constrain(day + direction, 1, 31);
        break;
      case SET_HOUR:
        hour = constrain(hour + direction, 0, 23);
        break;
      case SET_MINUTE:
        minute = constrain(minute + direction, 0, 59);
        break;
      case DONE:
        break;
      }
      buttonHandled = true;
      lastStep = DONE; // Force display update
    }

    if (buttonHandled) {
      delay(inputDebounceDelay);
    } else {
      delay(10); // Small delay when no button pressed
    }
  }

  RtcDateTime newTime(year, month, day, hour, minute, 0);
  rtc.SetDateTime(newTime);

  lcd.clear();
  printMessage(0, 0, "Time Set!");
  delay(exitDelay);
}

/**
 * @brief Calibrates pump timing for accurate water dispensing
 * @details Interactive calibration process to determine timing for 1 cup of
 * water:
 * - User sets test duration
 * - System runs pump for specified time
 * - User confirms if output equals 1 cup
 * - Saves calibration value for automatic watering calculations
 */
void waterCalibrationTest() {
  unsigned long waterTestDuration = 30000UL;

  enum SettingStep { SET_DURATION, DONE };
  SettingStep step = SET_DURATION;

  while (step != DONE) {
    lcd.clear();
    printMessage(0, 0, "Water Duration");
    if (step == SET_DURATION) {
      printMessage(0, 1, "(sec): " + waterTestDuration / 1000);
    }

    while (true) {
      // Handle increment/decrement buttons
      int direction = 0;
      if (isButtonPressed(buttonPins[0]))
        direction = -1;
      if (isButtonPressed(buttonPins[1]))
        direction = 1;

      if (direction != 0 && step == SET_DURATION) {
        waterTestDuration =
            constrain(waterTestDuration + (direction * 1000), 1000, 60000);
        break;
      }

      if (isButtonPressed(buttonPins[2])) {
        lcd.clear();
        lcd.print(waterTestDuration);
        delay(4000);
        step = static_cast<SettingStep>(step + 1);
        break;
      }

      if (isButtonPressed(buttonPins[3])) {
        printExitCurrentMenu();
        return;
      }
    }
    delay(200);
  }

  bool calibrationDone = false;

  while (!calibrationDone) {
    while (step != DONE) {
      lcd.clear();
      printMessage(0, 0, "Water Duration");
      if (step == SET_DURATION) {
        printMessage(0, 1, "(sec): " + String(waterTestDuration / 1000));
      }

      while (true) {
        // Handle increment/decrement buttons
        int direction = 0;
        if (isButtonPressed(buttonPins[0]))
          direction = -1;
        if (isButtonPressed(buttonPins[1]))
          direction = 1;

        if (direction != 0 && step == SET_DURATION) {
          waterTestDuration =
              constrain(waterTestDuration + (direction * 1000), 1000, 60000);
          break;
        }

        if (isButtonPressed(buttonPins[2])) {
          step = static_cast<SettingStep>(step + 1);
          break;
        }

        if (isButtonPressed(buttonPins[3])) {
          printExitCurrentMenu();
          return;
        }
      }
      delay(200);
    }

    // Ask user to start calibration test
    lcd.clear();
    printMessage(0, 0, "Start Cal Test?");
    printMessage(0, 1, "(-)=No (+)=Yes");
    delay(100);

    while (true) {
      if (isButtonPressed(buttonPins[0])) {
        printExitCurrentMenu();
        return;
      }

      if (isButtonPressed(buttonPins[1])) {
        // Run the water test
        if (isPlantOkayToWater()) {
          lcd.clear();
          printMessage(0, 0, "Dispensing..");
          printMessage(0, 1, "Please Wait!");
          digitalWrite(pumpValvePin, HIGH);
          delay(pumpValveTiming);
          analogWrite(pumpPin, pumpHighSetting);
          delay(waterTestDuration);
          analogWrite(pumpPin, 0);
          delay(pumpValveTiming);
          digitalWrite(pumpValvePin, LOW);
          lcd.clear();
          printMessage(0, 0, "Done!");
          delay(exitDelay);
        }

        // Ask if output was 1 cup
        lcd.clear();
        printMessage(0, 0, "1 cup output?");
        printMessage(0, 1, "(-)=No (+)=Yes");

        while (true) {
          if (isButtonPressed(buttonPins[0])) {
            // User says no - ask if they want to retry
            lcd.clear();
            printMessage(0, 0, "Retry test?");
            printMessage(0, 1, "(-)=No (+)=Yes");

            while (true) {
              if (isButtonPressed(buttonPins[0])) {
                printExitCurrentMenu();
                return;
              }
              if (isButtonPressed(buttonPins[1])) {
                step = SET_DURATION; // Go back to duration setting
                calibrationDone = false;
                break;
              }
            }
            break;
          }

          if (isButtonPressed(buttonPins[1])) {
            // User says yes - save calibration
            oneCupCalibrated = waterTestDuration;
            lcd.clear();
            printMessage(0, 0, "1Cup Calibration");
            delay(500);
            printMessage(0, 1, "    Saved!");
            delay(2000);
            lcd.clear();
            printMessage(0, 0, String(oneCupCalibrated));
            delay(4000);
            return;
          }
        }
        break;
      }
    }
  }
}

/**
 * @brief Toggles instruction message display setting
 * @details Allows user to enable/disable helpful tip messages throughout the
 * interface Provides immediate feedback when setting is changed
 */
void disableMessages() {
  lcd.clear();
  printMessage(0, 0, "Show Tips?");
  printMessage(0, 1, "(-)= No (+)=Yes");

  while (true) {
    if (isButtonPressed(buttonPins[0])) {
      delay(inputDebounceDelay);
      if (isButtonPressed(buttonPins[0])) {
        showInstructions = false;
        lcd.clear();
        printMessage(0, 0, "  Tip messages:  ");
        delay(500);
        printMessage(0, 1, "  Disabled     ");
        delay(1000);
        return;
      }
    }
    if (isButtonPressed(buttonPins[1])) {
      delay(inputDebounceDelay);
      if (isButtonPressed(buttonPins[1])) {
        showInstructions = true;
        printInstructions();
        printMessage(0, 0, "M: Confirm/Next");
        printMessage(0, 1, "A: Cancel");
        delay(transitionDelay);
        return;
      }
    }
  }
}

/**
 * @brief Reads soil moisture sensor with caching optimization
 * @return Soil moisture percentage as float (0.0-100.0)
 * @details Implements smart caching to reduce sensor reads and improve
 * performance:
 * - Caches readings for 5 seconds to avoid excessive ADC calls
 * - Converts analog reading (0-1023) to moisture percentage
 * - Higher analog values indicate drier soil (inverted scale)
 * - Returns cached value if called within caching window
 */
float readSoilMoisture() {
  static unsigned long lastReading = 0;
  static float lastMoisture = 0;
  unsigned long now = millis();

  // Cache reading for 1 second to avoid unnecessary sensor reads
  if (now - lastReading < 1000) {
    return lastMoisture;
  }

  digitalWrite(pinSoilPower, HIGH);
  delay(10); // Small delay for sensor stabilization
  lastRawMoistureValue = analogRead(pinSoilRead);
  digitalWrite(pinSoilPower, LOW);

  lastMoisture = calculateMoisture(lastRawMoistureValue);
  lastReading = now;

  return lastMoisture;
}

/**
 * @brief Converts raw ADC reading to moisture percentage
 * @param raw Raw analog reading from moisture sensor (0-1023)
 * @return Moisture percentage (0-100)
 * @details Maps sensor reading between dry and wet calibration values
 */
unsigned char calculateMoisture(unsigned int raw) {
  if (raw <= DRY_VALUE)
    return 0;
  if (raw >= WET_VALUE)
    return 100;

  return map(raw, DRY_VALUE, WET_VALUE, 0, 100);
}

/**
 * @brief Checks water level sensor status
 * @return true if water level is low, false if adequate
 * @details Simple digital read from water level sensor pin
 */
bool isWaterDetected() { return digitalRead(waterSensorPin) == HIGH; }

/**
 * @brief Comprehensive safety check before watering
 * @return true if safe to water, false if conditions prevent watering
 * @details Checks multiple safety conditions:
 * - Soil moisture level (prevents overwatering)
 * - Water detection sensor (prevents flooding)
 * - Displays appropriate warning messages
 */
bool isPlantOkayToWater() {
  digitalWrite(waterDetectionPower, HIGH);
  delay(sensorWarmTime);
  unsigned int waterDetectionValue = analogRead(waterDetectionRead);

  moistureLevel = readSoilMoisture();
  if (moistureLevel >= 70) {
    lcd.clear();
    printMessage(0, 0, "Soil already wet!");
    printMessage(0, 1, getMoistureValue());
    delay(3000);
    lcd.clear();
    return false;
  }

  if (waterDetectionValue > waterDetectThreshold) {
    lcd.clear();
    printMessage(0, 0, "WATER DETECTED!!");
    printMessage(0, 1, "TRY AGAIN LATER");
    delay(3000);
    lcd.clear();
    return false;
  }

  digitalWrite(waterDetectionPower, LOW);
  return true;
}

/**
 * @brief Display message at specific LCD coordinates
 * @param x Column position (0-15 for 16x2 LCD)
 * @param y Row position (0-1 for 16x2 LCD)
 * @param message Text string to display
 * @details Simple utility function for positioning text on LCD display
 * Sets cursor position and prints the provided message
 */
void printMessage(int x, int y, const String &message) {
  lcd.setCursor(x, y);
  lcd.print(message);
}

/**
 * @brief Animated text display with typewriter effect
 * @param message Text to display with animation
 * @details Creates a typewriter effect by printing characters one by one
 * with a 100ms delay between each character. Uses blinking cursor for
 * visual feedback and positions text on the second row of LCD
 */
void printAnimation(String message) {
  lcd.setCursor(0, 1);
  lcd.blink();
  for (unsigned int i = 0; i < message.length(); ++i) {
    lcd.print(message.charAt(i));
    delay(bootAnimationDelay);
  }
}

/**
 * @brief Displays standard exit message when leaving menus
 * @details Shows a friendly "Please Wait" message followed by "Exiting"
 * with appropriate delays for user feedback. Provides consistent
 * exit experience across all menu functions
 */
void printExitCurrentMenu() {
  lcd.clear();
  printMessage(0, 0, "Please Wait ^_^ ");
  delay(200);
  printMessage(0, 1, "    Exiting");
  delay(exitDelay);
}

/**
 * @brief Displays control instructions for interactive menus
 * @details Shows standardized instruction text explaining button usage:
 * - How to use +/- buttons for value changes
 * - General navigation help for menu systems
 * Used across multiple menu functions for consistency
 */
void printInstructions() {
  lcd.clear();
  printMessage(0, 0, "Use buttons to:");
  printMessage(0, 1, "-/+ to change");
  delay(transitionDelay);

  lcd.clear();
  printMessage(0, 0, "M: Confirm/Next");
  printMessage(0, 1, "A: Exit");
  delay(transitionDelay);
  lcd.clear();
}

/**
 * @brief Converts 24-hour time to 12-hour format with AM/PM
 * @param hour Reference to hour value (modified in place)
 * @param isPM Reference to PM flag (set to true for PM times)
 * @details Handles midnight (0) and noon (12) edge cases properly
 */
void formatTime(int &hour, bool &isPM) {
  if (hour == 0) {
    hour = 12;
  } else if (hour >= 12) {
    if (hour > 12)
      hour -= 12;
    isPM = true;
  }
}

/**
 * @brief Formats soil moisture reading as percentage string
 * @return Formatted moisture string with proper spacing and % symbol
 * @details Converts the last raw moisture reading to a user-friendly format:
 * - Rounds to nearest whole percentage
 * - Constrains value to 0-100% range
 * - Provides consistent spacing (single-digit: "  5%", double: " 85%")
 * - Returns as String for easy LCD display
 */
String getMoistureValue() {
  unsigned char mP = calculateMoisture(lastRawMoistureValue);

  char buffer[5]; // " 99%\0"
  if (mP < 10) {
    sprintf(buffer, "  %d%%", mP);
  } else if (mP < 100) {
    sprintf(buffer, " %d%%", mP);
  } else {
    sprintf(buffer, "%d%%", mP);
  }

  return String(buffer);
}

/**
 * @brief Formats countdown time for next watering
 * @param totalSecondsRemaining Total seconds until next watering
 * @param hoursPart Hours component of remaining time
 * @param minutesPart Minutes component of remaining time
 * @return Formatted time string (e.g., "2H30M" or "45 Sec")
 * @details Provides compact time display for LCD with different formats
 * based on time remaining (seconds vs hours/minutes)
 */
String getNextFeed(const unsigned long totalSecondsRemaining,
                   const unsigned long hoursPart,
                   const unsigned long minutesPart) {
  if (totalSecondsRemaining < 60) {
    char buffer[7]; // "999 Sec\0"
    sprintf(buffer, "%lu Sec", totalSecondsRemaining);
    return String(buffer);
  }

  char buffer[8]; // " 99H99M\0"
  if (hoursPart < 10) {
    sprintf(buffer, " %luH%02luM", hoursPart, minutesPart);
  } else {
    sprintf(buffer, "%luH%02luM", hoursPart, minutesPart);
  }

  return String(buffer);
}

/**
 * @brief Formats current time with blinking colon separator
 * @param hour Hour in 12-hour format
 * @param isPM AM/PM indicator
 * @return Formatted time string (e.g., "02:30 PM" or "02 30 PM")
 * @details Uses global showColon variable to create blinking effect
 */
String getTime(const RtcDateTime now) {
  bool isPM = false;
  int hour = now.Hour();
  formatTime(hour, isPM);
  char buffer[9]; // "99:99 PM\0"
  char separator = showColon ? ':' : ' ';
  sprintf(buffer, "%02d%c%02d %s", hour, separator, now.Minute(),
          isPM ? "PM" : "AM");
  return String(buffer);
}

/**
 * @brief Formats current date from RTC as display string
 * @return Formatted date string (MM/DD/YYYY format)
 * @details Retrieves current date from DS1302 RTC and formats for LCD
 * display:
 * - Uses MM/DD/YYYY format with zero-padding
 * - Handles month/day values less than 10 with leading zeros
 * - Returns as String for consistent display formatting
 */
String getDate(const RtcDateTime now) {
  char buffer[11]; // "99/99/9999\0"
  sprintf(buffer, "%02d/%02d/%04d", now.Month(), now.Day(), now.Year());
  return String(buffer);
}
