#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <RtcDS1302.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte CLK = 7;
const byte DAT = 6;
const byte RST = 8;
ThreeWire rtcWire(DAT, CLK, RST);
RtcDS1302<ThreeWire> rtc(rtcWire);

const byte buttonPins[] = {2, 3, 4, 5};
const byte totalButtons = sizeof(buttonPins) / sizeof(buttonPins[0]);

const int pinSoilPower = 11;
const int pinSoilRead = A2;
const int waterDetectionRead = A3;
const int waterDetectionPower = 13;

const int waterSensorPin = 12;
const int DRY_VALUE = 300;
const int WET_VALUE = 880;
const int READ_DELAY = 200;
const int waterDetectThreshold = 350;
unsigned long lastRawMoistureValue = 0;

const int pumpValvePin = 9;
const int pumpPin = 10;
const int pumpValveTiming = 2000;
const int pumpLowSetting = 90;
const int pumpMidSetting = 150;
const int pumpHighSetting = 255;
unsigned long waterInterval = 0;
unsigned long waterIntervalHour = 60;
unsigned long waterIntervalDelta = 60;
unsigned long waterDuration = 20000UL;

float waterCupsTarget = 1.0;
float moistureLevel = 0.0;
unsigned long waterIntervalHours = 24;
unsigned long lastWaterTime = 0;
unsigned long oneCupCalibrated = 0;
unsigned long autoWaterDurationMillis = 0;

const unsigned long messageDisplayDuration = 3000;
const unsigned long dateAndCountDownDelay = 6000;
const unsigned long inputDebounceDelay = 25;
const unsigned long bootAnimationDelay = 50;
const unsigned long bootWait = 3500;
const unsigned long transitionDelay = 2000;
const unsigned long exitDelay = 1000;
const unsigned long sensorWarmTime = 200;
const unsigned long blinkInterval = 500;

unsigned long lastTimeButtonStateChanged = millis();
unsigned long debounceDuration = 100;
byte lastButtonState;

bool isAutoModeEnabled = false;
bool recentBranch = false;
bool showInstructions = false;
bool displayDateMessage = true;
byte currentMenu = 0;
byte messageIndex = 0;
unsigned long lastMessageSwitch = 0;
static bool showColon = true;
unsigned long lastBlink = 0;
unsigned long autoTimer = 0;

String messagesHomeScreen[] = {
    "(-) Show Clock  ",
    "(+) Settings   ",
    "(M) Manual Mode ",
    "(A) Auto Mode   ",
};
const byte totalMessages =
    sizeof(messagesHomeScreen) / sizeof(messagesHomeScreen[0]);

void displayStartup();
void showMessageCycle();
void showMessageCycleClock();
void checkButtons();
bool isButtonPressed(byte pin);
void handleMenu(byte menu);
void settingsMenu();
void showClock();
void setDateTime();
void manualWatering();
void autoWatering();
void waterPlant();
float readSoilMoisture();
int calculateMoisture(int raw);
bool isWaterDetected();
void waterCalibrationTest();
void disableMessages();
void autoWateringCheck();
bool isPlantOkayToWater();
void formatTime(int &hour, bool &isPM);
void printTime(const int hour, const bool isPM);
void printMoistureValue();
void printAutoModeDate(RtcDateTime t);
void printNextFeed(unsigned long totalSecondsRemaining, unsigned long hoursPart,
                   unsigned long minutesPart);

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("  Created by:   ");
  lcd.setCursor(0, 1);
  lcd.print("  [Aqua");
  lcd.write(243);
  lcd.print("Works]");
  delay(4000);

  for (byte i = 0; i < totalButtons; ++i) {
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

void loop() {

  if (isWaterDetected()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Water Lvl Low!");
    lcd.setCursor(0, 1);
    lcd.print("Please Add Water :)");
    delay(5000);
  } else if (currentMenu == 0) {
    showMessageCycle();
    checkButtons();
  } else {
    handleMenu(currentMenu);
    currentMenu = 0;
  }
}

void displayStartup() {
  String title = "Water Pump Menu";

  lcd.setCursor(0, 0);
  lcd.print(title);
  lcd.setCursor(0, 1);
  lcd.blink();

  String loading = "    Loading....";
  for (int i = 0; i < loading.length(); ++i) {
    lcd.print(loading.charAt(i));
    delay(bootAnimationDelay);
  }

  delay(bootWait);
}

void showMessageCycle() {
  if (millis() - lastMessageSwitch >= messageDisplayDuration) {
    lcd.noBlink();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" ");
    lcd.write(243);
    lcd.print(" MainMenu ");
    lcd.write(243);
    lcd.setCursor(0, 1);
    lcd.print(messagesHomeScreen[messageIndex]);

    messageIndex = (messageIndex + 1) % totalMessages;
    lastMessageSwitch = millis();
  }
}

void formatTime(int &hour, bool &isPM) {
  if (hour == 0) {
    hour = 12;
  } else if (hour >= 12) {
    if (hour > 12)
      hour -= 12;
    isPM = true;
  }
}

void printTime(const int hour, const bool isPM) {
  lcd.setCursor(0, 0);
  if (hour < 10)
    lcd.print('0');
  lcd.print(hour);
  lcd.print(showColon ? ':' : ' ');
  if (now.Minute() < 10)
    lcd.print('0');
  lcd.print(now.Minute());
  lcd.print(isPM ? " PM" : " AM");
}

void printMoistureValue() {
  int mP = int(calculateMoisture(lastRawMoistureValue) + 0.5);
  if (mP < 100)
    lcd.print(' ');
  if (mP < 10)
    lcd.print(' ');
  lcd.print(mP);
  lcd.print('%');
}

void printNextFeed(unsigned long totalSecondsRemaining, unsigned long hoursPart,
                   unsigned long minutesPart) {
  if (totalSecondsRemaining > 60) {
    if (hoursPart < 10) {
      lcd.print(" ");
    }
    lcd.print(hoursPart);
    lcd.print("H");

    if (minutesPart < 10) {
      lcd.print("0");
    }
    lcd.print(minutesPart);
    lcd.print("M");
  } else {
    lcd.setCursor(10, 1);
    lcd.print(totalSecondsRemaining);
    lcd.print(" Sec");
  }
}

void printAutoModeDate(RtcDateTime t) {
  if (t.Month() < 10)
    lcd.print('0');
  lcd.print(t.Month());
  lcd.print('/');
  if (t.Day() < 10)
    lcd.print('0');
  lcd.print(t.Day());
  lcd.print('/');
  lcd.print(t.Year());
}

void printManualModeDate(RtcDateTime t) {
  if (t.Month() < 10)
    lcd.print('0');
  lcd.print(t.Month());
  lcd.print('/');
  if (t.Day() < 10)
    lcd.print('0');
  lcd.print(t.Day());
  lcd.print('/');
  lcd.print(t.Year());
}

void showMessageCycleClock() {
  unsigned long nowMs = millis();

  if (nowMs - lastBlink >= blinkInterval) {
    showColon = !showColon;
    lastBlink = nowMs;
  }

  RtcDateTime now = rtc.GetDateTime();
  RtcDateTime t = rtc.GetDateTime();
  bool isPM = false;
  int hour = now.Hour();
  formatTime(hour, isPM);
  printTime(hour, isPM);

  lcd.setCursor(10, 0);
  printMoistureValue();

  if (!isAutoModeEnabled) {
    lcd.setCursor(0, 1);
    printManualModeDate(t);
  } else {
    lcd.setCursor(0, 1);

    if (millis() - lastMessageSwitch >= dateAndCountDownDelay) {
      displayDateMessage = !displayDateMessage;
      lastMessageSwitch = nowMs;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
    }

    if (displayDateMessage) {
      lcd.setCursor(0, 1);
      printAutoModeDate(t);
    } else {
      unsigned long elaspedTime = nowMs - autoTimer;
      unsigned long remainingTime = 0;
      unsigned long intervalMilli = waterInterval * 1000;

      if (elaspedTime < intervalMilli) {
        remainingTime = intervalMilli - elaspedTime;
      }

      unsigned long totalSecondsRemaining = remainingTime / 1000;
      unsigned long hoursPart = totalSecondsRemaining / 3600UL;
      unsigned long minutesPart = (totalSecondsRemaining % 3600UL) / 60UL;
      lcd.print("Feeds in: ");
      printNextFeed(totalSecondsRemaining, hoursPart, minutesPart);
    }
  }
}

void checkButtons() {
  for (byte i = 0; i < totalButtons; ++i) {
    if (isButtonPressed(buttonPins[i])) {
      currentMenu = i + 1;
      break;
    }
  }
}

bool isButtonPressed(byte pin) {
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

void handleMenu(byte menu) {
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

void settingsMenu() {
  if (showInstructions) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Use buttons to:");
    lcd.setCursor(0, 1);
    lcd.print("-/+ to change");
    delay(transitionDelay);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("M: Confirm/Next");
    lcd.setCursor(0, 1);
    lcd.print("A: Exit");
    delay(transitionDelay);
  }

  const byte totalSettings = 3;
  byte selected = 0;

  String options[totalSettings] = {"1.Set Time/Date", "2.Calibrate Test",
                                   "3.Disable Msgs"};

  while (true) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Select Option:");
    lcd.setCursor(0, 1);
    lcd.print(options[selected]);

    delay(inputDebounceDelay);
    while (true) {
      if (isButtonPressed(buttonPins[0])) {
        while (digitalRead(buttonPins[0]) == LOW)
          ;
        selected = (selected == 0) ? totalSettings - 1 : selected - 1;
        break;
      } else if (isButtonPressed(buttonPins[1])) {
        while (digitalRead(buttonPins[1]) == LOW)
          ;
        selected = (selected + 1) % totalSettings;
        break;
      } else if (isButtonPressed(buttonPins[2])) {
        while (digitalRead(buttonPins[2]) == LOW)
          ;

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
      } else if (isButtonPressed(buttonPins[3])) {
        while (digitalRead(buttonPins[3]) == LOW)
          ;
        lcd.clear();
        lcd.print("Please Wait ^_^ ");
        lcd.setCursor(0, 1);
        delay(200);
        lcd.print("    Exiting");
        delay(exitDelay);
        return;
      }
    }

    delay(50);
  }
}

void showClock() {
  if (showInstructions) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Use buttons to:");
    lcd.setCursor(0, 1);
    lcd.print("-/+ to change");
    delay(transitionDelay);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("M: Confirm/Next");
    lcd.setCursor(0, 1);
    lcd.print("A: Exit");
    delay(transitionDelay);
    lcd.clear();
  }

  String moistureCheck = "  Measuring...  ";
  displayDateMessage = true;
  lastMessageSwitch = millis();
  lcd.clear();

  while (true) {
    showMessageCycleClock();
    autoWateringCheck();

    if (digitalRead(buttonPins[2]) == LOW) {
      delay(inputDebounceDelay);
      if (digitalRead(buttonPins[2]) == LOW) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Moisture Lvl:");
        lcd.setCursor(0, 1);
        lcd.blink();

        for (int i = 0; i < moistureCheck.length(); ++i) {
          lcd.print(moistureCheck.charAt(i));
          delay(bootAnimationDelay);
        }

        readSoilMoisture();
        delay(messageDisplayDuration);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("      Done     ");
        lcd.clear();
        lcd.noBlink();
        lcd.clear();
      }
    } else if (digitalRead(buttonPins[3]) == LOW) {
      delay(inputDebounceDelay);
      if (digitalRead(buttonPins[3]) == LOW) {
        if (isAutoModeEnabled) {
          lcd.clear();
          lcd.print("  [Auto Mode]");
          lcd.setCursor(0, 1);
          delay(500);
          lcd.print("  Disabled :(");
          isAutoModeEnabled = false;
          delay(2000);
        }
        lcd.clear();
        lcd.print("Please Wait ^_^ ");
        lcd.setCursor(0, 1);
        delay(200);
        lcd.print("    Exiting");
        delay(exitDelay);
        return;
      }
    }
    delay(inputDebounceDelay);
  }
}

void setDateTime() {
  int year = 2025;
  int month = 1;
  int day = 1;
  int hour = 12;
  int minute = 0;

  enum Step { SET_YEAR, SET_MONTH, SET_DAY, SET_HOUR, SET_MINUTE, DONE };
  Step step = SET_YEAR;

  while (step != DONE) {
    lcd.clear();

    switch (step) {
    case SET_YEAR:
      lcd.setCursor(0, 0);
      lcd.print("Set Year: ");
      lcd.print(year);
      break;
    case SET_MONTH:
      lcd.setCursor(0, 0);
      lcd.print("Set Month: ");
      lcd.print(month);
      break;
    case SET_DAY:
      lcd.setCursor(0, 0);
      lcd.print("Set Day: ");
      lcd.print(day);
      break;
    case SET_HOUR:
      lcd.setCursor(0, 0);
      lcd.print("Set Hour: ");
      lcd.print(hour);
      break;
    case SET_MINUTE:
      lcd.setCursor(0, 0);
      lcd.print("Set Minute: ");
      lcd.print(minute);
      break;
    default:
      break;
    }

    lcd.setCursor(0, 1);
    lcd.print("(-)(+)(M)Next");

    while (true) {
      if (isButtonPressed(buttonPins[0])) {
        switch (step) {
        case SET_YEAR:
          if (year > 2000)
            --year;
          break;
        case SET_MONTH:
          if (month > 1)
            --month;
          break;
        case SET_DAY:
          if (day > 1)
            --day;
          break;
        case SET_HOUR:
          if (hour > 0)
            --hour;
          break;
        case SET_MINUTE:
          if (minute > 0)
            --minute;
          break;
        }
        break;
      } else if (isButtonPressed(buttonPins[1])) {
        switch (step) {
        case SET_YEAR:
          if (year < 2099)
            ++year;
          break;
        case SET_MONTH:
          if (month < 12)
            ++month;
          break;
        case SET_DAY:
          if (day < 31)
            ++day;
          break;
        case SET_HOUR:
          if (hour < 23)
            ++hour;
          break;
        case SET_MINUTE:
          if (minute < 59)
            ++minute;
          break;
        }
        break;
      } else if (isButtonPressed(buttonPins[2])) {
        step = static_cast<Step>(step + 1);
        break;
      } else if (isButtonPressed(buttonPins[3])) {
        lcd.clear();
        lcd.print("Please Wait ^_^ ");
        lcd.setCursor(0, 1);
        delay(200);
        lcd.print("    Exiting");
        delay(exitDelay);
        return;
      }
    }

    delay(inputDebounceDelay);
  }

  RtcDateTime newTime(year, month, day, hour, minute, 0);
  rtc.SetDateTime(newTime);

  lcd.clear();
  lcd.print("Time Set!");
  delay(exitDelay);
}

void manualWatering() {
  if (showInstructions) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Use buttons to:");
    lcd.setCursor(0, 1);
    lcd.print("-/+ to change");
    delay(transitionDelay);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("M: Confirm/Next");
    lcd.setCursor(0, 1);
    lcd.print("A: Exit");
    delay(transitionDelay);
  }

  delay(500);

  while (isPlantOkayToWater()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(byte(243));
    lcd.print(" Manual Water ");
    lcd.write(byte(243));

    lcd.setCursor(0, 1);
    lcd.print("(M)Hold (A):Esc ");

    bool currentlyWatering = false;

    while (true) {
      delay(inputDebounceDelay);
      bool isMHeld = (digitalRead(buttonPins[2]) == LOW);

      if (isMHeld != currentlyWatering) {
        currentlyWatering = isMHeld;

        lcd.setCursor(0, 1);

        if (currentlyWatering && isPlantOkayToWater()) {
          lcd.print("  Watering...   ");
          digitalWrite(pumpValvePin, HIGH);
          analogWrite(pumpPin, pumpHighSetting);
        } else {
          lcd.print("(M)Hold (A):Esc ");
          analogWrite(pumpPin, 0);
          digitalWrite(pumpValvePin, LOW);
        }
      }

      if (isButtonPressed(buttonPins[3])) {
        if (currentlyWatering) {
          analogWrite(pumpPin, 0);
          digitalWrite(pumpValvePin, LOW);
        }

        lcd.clear();
        lcd.print("Please Wait ^_^ ");
        lcd.setCursor(0, 1);
        delay(200);
        lcd.print("    Exiting");
        delay(exitDelay);
        return;
      }

      delay(50);
    }
  }
}

void autoWatering() {
  if (showInstructions) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Use buttons to:");
    lcd.setCursor(0, 1);
    lcd.print("-/+ to change");
    delay(transitionDelay);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("M: Confirm/Next");
    lcd.setCursor(0, 1);
    lcd.print("A: Exit");
    delay(transitionDelay);
  }

  enum SettingStep { SET_VALUE, SET_FREQUENCY, DONE };
  SettingStep step = SET_VALUE;

  float targetCups = 1.0;
  float stepp = 0.5;

  if (oneCupCalibrated <= 0) {
    lcd.clear();
    lcd.print("Calibration");
    lcd.setCursor(0, 1);
    lcd.print("Needed...");
    delay(1500);
    waterCalibrationTest();
  }

  if (oneCupCalibrated <= 0) {
    return;
  }

  delay(500);

  while (step != DONE) {
    lcd.clear();
    if (step == SET_VALUE) {
      lcd.setCursor(0, 0);
      lcd.print("How much water?");
      lcd.setCursor(0, 1);
      lcd.print("Cups: ");
      lcd.print(targetCups, 1);

      while (true) {
        if (isButtonPressed(buttonPins[0])) {
          while (digitalRead(buttonPins[0]) == LOW)
            ;
          if (targetCups > stepp) {
            targetCups -= stepp;
          } else {
            targetCups = 0.5;
          }
          break;
        } else if (isButtonPressed(buttonPins[1])) {
          while (digitalRead(buttonPins[1]) == LOW)
            ;
          targetCups += stepp;
          break;
        } else if (isButtonPressed(buttonPins[2])) {
          autoWaterDurationMillis =
              (unsigned long)(targetCups * oneCupCalibrated);
          waterDuration = autoWaterDurationMillis;
          lcd.clear();
          lcd.print(waterDuration);
          delay(2000);
          step = SET_FREQUENCY;
          break;
        } else if (isButtonPressed(buttonPins[3])) {
          lcd.clear();
          lcd.print("Please Wait ^_^ ");
          lcd.setCursor(0, 1);
          delay(200);
          lcd.print("    Exiting");
          delay(exitDelay);
          return;
        }
      }
      delay(50);
    } else if (step == SET_FREQUENCY) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("How frequent?");
      lcd.setCursor(0, 1);

      lcd.print("Minutes: ");
      lcd.print(waterIntervalHour / 60);

      while (true) {
        if (isButtonPressed(buttonPins[0])) {
          if (waterIntervalHour >= waterIntervalDelta)
            waterIntervalHour -= waterIntervalDelta;
          break;
        } else if (isButtonPressed(buttonPins[1])) {
          waterIntervalHour += waterIntervalDelta;
          break;
        } else if (isButtonPressed(buttonPins[2])) {
          waterInterval = waterIntervalHour;
          step = DONE;
          break;
        } else if (isButtonPressed(buttonPins[3])) {
          lcd.clear();
          lcd.print("Please Wait ^_^ ");
          lcd.setCursor(0, 1);
          delay(200);
          lcd.print("    Exiting");
          delay(exitDelay);
          return;
        }
      }
      delay(100);
    }
  }

  isAutoModeEnabled = true;
  autoTimer = 0;
  lcd.clear();
  lcd.print("  [Auto Mode]");
  lcd.setCursor(0, 1);
  delay(500);
  lcd.print("  Enabled :)");
  delay(2000);
  autoTimer = millis();

  showClock();
}

void waterPlant() {
  if (isPlantOkayToWater()) {
    lcd.clear();
    lcd.print("Watering plant..");
    digitalWrite(pumpValvePin, HIGH);
    delay(pumpValveTiming);
    analogWrite(pumpPin, pumpHighSetting);
    delay(waterDuration);
    analogWrite(pumpPin, 0);
    delay(pumpValveTiming);
    digitalWrite(pumpValvePin, LOW);
    lcd.clear();
    lcd.print("Done!");
    delay(exitDelay);
    lcd.clear();
  }
}

float readSoilMoisture() {
  digitalWrite(pinSoilPower, HIGH);
  delay(READ_DELAY);

  lastRawMoistureValue = analogRead(pinSoilRead);
  delay(READ_DELAY);
  digitalWrite(pinSoilPower, LOW);

  float moisturePercent = calculateMoisture(lastRawMoistureValue);

  return moisturePercent;
}

int calculateMoisture(int raw) {
  if (raw <= DRY_VALUE)
    return 0;
  if (raw >= WET_VALUE)
    return 100;

  return map(raw, DRY_VALUE, WET_VALUE, 0, 100);
}

bool isWaterDetected() { return digitalRead(waterSensorPin) == HIGH; }

void waterCalibrationTest() {
  unsigned long waterTestDuration = 30000UL;

  enum SettingStep { SET_DURATION, DONE };
  SettingStep step = SET_DURATION;

  while (step != DONE) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Water Duration");
    if (step == SET_DURATION) {
      lcd.setCursor(0, 1);
      lcd.print("(sec): ");
      lcd.print(waterTestDuration / 1000);
    }

    while (true) {
      if (isButtonPressed(buttonPins[0])) {
        if (step == SET_DURATION && waterTestDuration >= 1000)
          waterTestDuration -= 1000;
        break;
      } else if (isButtonPressed(buttonPins[1])) {
        if (step == SET_DURATION)
          waterTestDuration += 1000;
        break;
      } else if (isButtonPressed(buttonPins[2])) {
        lcd.clear();
        lcd.print(waterTestDuration);
        delay(4000);
        step = static_cast<SettingStep>(step + 1);
        break;
      } else if (isButtonPressed(buttonPins[3])) {
        lcd.clear();
        lcd.print("Please Wait ^_^ ");
        lcd.setCursor(0, 1);
        delay(200);
        lcd.print("    Exiting");
        delay(exitDelay);
        return;
      }
    }
    delay(200);
  }

  bool calibrationDone = false;

  while (!calibrationDone) {
    while (step != DONE) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Water Duration");
      if (step == SET_DURATION) {
        lcd.setCursor(0, 1);
        lcd.print("(sec): ");
        lcd.print(waterTestDuration / 1000);
      }

      while (true) {
        if (isButtonPressed(buttonPins[0])) {
          if (step == SET_DURATION && waterTestDuration >= 1000)
            waterTestDuration -= 1000;
          break;
        } else if (isButtonPressed(buttonPins[1])) {
          if (step == SET_DURATION)
            waterTestDuration += 1000;
          break;
        } else if (isButtonPressed(buttonPins[2])) {
          lcd.clear();
          lcd.print(waterTestDuration);
          delay(4000);
          step = static_cast<SettingStep>(step + 1);
          break;
        } else if (isButtonPressed(buttonPins[3])) {
          lcd.clear();
          lcd.print("Please Wait ^_^ ");
          lcd.setCursor(0, 1);
          delay(200);
          lcd.print("    Exiting");
          delay(exitDelay);
          return;
        }
      }
      delay(200);
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Start Cal Test?");
    lcd.setCursor(0, 1);
    lcd.print("(-)=No (+)=Yes");
    delay(100);

    while (true) {
      if (isButtonPressed(buttonPins[0])) {
        lcd.clear();
        lcd.print("Please Wait ^_^ ");
        lcd.setCursor(0, 1);
        delay(200);
        lcd.print("    Exiting");
        delay(exitDelay);
        return;
      } else if (isButtonPressed(buttonPins[1])) {

        if (isPlantOkayToWater()) {
          lcd.clear();
          lcd.print("Dispensing..");
          lcd.setCursor(0, 1);
          lcd.print("Please Wait!");
          digitalWrite(pumpValvePin, HIGH);
          delay(pumpValveTiming);
          analogWrite(pumpPin, pumpHighSetting);
          delay(waterTestDuration);
          analogWrite(pumpPin, 0);
          delay(pumpValveTiming);
          digitalWrite(pumpValvePin, LOW);
          lcd.clear();
          lcd.print("Done!");
          delay(exitDelay);
          lcd.clear();
        }

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("1 cup output?");
        lcd.setCursor(0, 1);
        lcd.print("(-)=No (+)=Yes");

        while (true) {
          if (isButtonPressed(buttonPins[0])) {
            while (digitalRead(buttonPins[0]) == LOW)
              ;
            lcd.clear();
            lcd.print("Retry test?");
            lcd.setCursor(0, 1);
            lcd.print("(-)=No (+)=Yes");

            while (true) {
              if (isButtonPressed(buttonPins[0])) {
                while (digitalRead(buttonPins[0]) == LOW)
                  ;
                lcd.clear();
                lcd.print("Please Wait ^_^ ");
                lcd.setCursor(0, 1);
                delay(200);
                lcd.print("    Exiting");
                delay(exitDelay);
                delay(1000);
                calibrationDone = true;
                break;
              } else if (isButtonPressed(buttonPins[1])) {
                while (digitalRead(buttonPins[1]) == LOW)
                  ;
                step = SET_DURATION;
                break;
              }
            }
            break;
          } else if (isButtonPressed(buttonPins[1])) {
            while (digitalRead(buttonPins[0]) == LOW)
              ;
            oneCupCalibrated = waterTestDuration;
            lcd.clear();
            lcd.print("1Cup Calibration");
            lcd.setCursor(0, 1);
            delay(500);
            lcd.print("    Saved!");
            delay(2000);
            lcd.clear();
            lcd.print(oneCupCalibrated);
            delay(4000);
            calibrationDone = true;
            break;
          }
        }
        break;
      }
    }
  }
}

void disableMessages() {
  lcd.clear();
  lcd.print("Show Tips?");
  lcd.setCursor(0, 1);
  lcd.print("(-)= No (+)=Yes");

  while (true) {
    if (isButtonPressed(buttonPins[0])) {
      delay(inputDebounceDelay);
      if (isButtonPressed(buttonPins[0])) {
        showInstructions = false;
        lcd.clear();
        lcd.print("  Tip messages:  ");
        lcd.setCursor(0, 1);
        delay(500);
        lcd.print("  Disabled     ");
        delay(1000);
        return;
      }
    } else if (isButtonPressed(buttonPins[1])) {
      delay(inputDebounceDelay);
      if (isButtonPressed(buttonPins[1])) {
        showInstructions = true;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Use buttons to:");
        lcd.setCursor(0, 1);
        lcd.print("-/+ to change");
        delay(transitionDelay);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("M: Confirm/Next");
        lcd.setCursor(0, 1);
        lcd.print("A: Cancel");
        delay(transitionDelay);
        return;
      }
    }
  }
}

void autoWateringCheck() {
  if (isAutoModeEnabled && (millis() - autoTimer >= waterInterval * 1000)) {
    waterPlant();
    autoTimer = millis();
  }
}

bool isPlantOkayToWater() {
  digitalWrite(waterDetectionPower, HIGH);
  delay(sensorWarmTime);
  int waterDetectionValue = analogRead(waterDetectionRead);

  moistureLevel = readSoilMoisture();
  if (moistureLevel > 70) {
    lcd.clear();
    lcd.print("Soil already wet!");
    lcd.setCursor(0, 1);
    lcd.print(String("Moisture: ") + (int)moistureLevel + "%");
    delay(3000);
    lcd.clear();
    return false;
  }

  if (waterDetectionValue > waterDetectThreshold) {
    lcd.clear();
    lcd.print("WATER DETECTED!!");
    lcd.setCursor(0, 1);
    lcd.print("TRY AGAIN LATER");
    delay(3000);
    lcd.clear();
    return false;
  }

  digitalWrite(waterDetectionPower, LOW);

  return true;
}
