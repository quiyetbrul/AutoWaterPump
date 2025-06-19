# AutoWaterPump 🌱💧

An Arduino-based automatic plant watering system with smart monitoring, scheduling, and manual control features.

## 📋 Overview

The AutoWaterPump is an intelligent plant watering system built on Arduino Uno that monitors soil moisture levels and automatically waters plants based on configurable schedules and thresholds. It features an LCD display, menu system, and multiple sensors to ensure optimal plant care.

## ✨ Features

### 🤖 Automatic Watering

- **Smart scheduling**: Set custom watering intervals (hours)
- **Soil moisture monitoring**: Real-time soil moisture detection
- **Water level detection**: Prevents dry pumping
- **Configurable pump speeds**: Low, medium, high settings
- **Target water amount**: Set precise watering volumes

### 📱 User Interface

- **16x2 LCD Display**: Clear status and menu navigation
- **4-button control**: Easy navigation and settings
- **Multiple menu pages**: Auto watering, manual control, settings, calibration, debug
- **Boot animation**: Visual startup sequence

### 🔧 Manual Control

- **Manual watering mode**: Override automatic system
- **Pump speed control**: Adjust flow rate manually
- **Real-time monitoring**: View sensor readings during operation

### ⚙️ Configuration & Calibration

- **EEPROM settings storage**: Persistent configuration
- **Sensor calibration**: Calibrate soil moisture thresholds
- **Debug mode**: Monitor system internals
- **Settings management**: Save/load user preferences

### 🕒 Real-Time Clock Integration

- **Scheduled watering**: Time-based watering schedules
- **Timing precision**: Accurate interval tracking
- **Status display**: Show next watering time

## 🔌 Hardware Requirements

### 📦 Core Components

- **Arduino Uno** (or compatible)
- **16x2 LCD Display** with I2C backpack
- **Water pump** (12V recommended)
- **Water level sensor**
- **Soil moisture sensor**
- **Real-time clock module** (DS1307/DS3231)
- **4 push buttons** for navigation

### 🔗 Pin Configuration

```text
Buttons: Digital pins 2, 3, 4, 5
LCD I2C: SDA (A4), SCL (A5)
RTC: CLK (7), DAT (6), RST (8)
Soil Moisture: Power (11), Read (A2)
Water Detection: Power (13), Read (A3)
Water Sensor: Digital pin 12
Pump Control: Digital pin 10
Pump Valve: Digital pin 9
```

### ⚡ Power Requirements

- **Arduino**: 5V via USB or barrel jack
- **Pump**: External 12V power supply
- **Sensors**: 5V from Arduino

## 📚 Dependencies

The project uses PlatformIO and requires the following libraries:

```ini
lib_deps =
    marcoschwartz/LiquidCrystal_I2C@^1.1.4
    adafruit/RTClib@^2.1.1
    arduino-libraries/ArduinoHttpClient@^0.4.0
```

## 🔧 Configuration

### Default Settings

```cpp
// Sensor thresholds
DRY_VALUE = 300
WET_VALUE = 880
WATER_DETECT_THRESHOLD = 350

// Pump speeds
PUMP_LOW = 90
PUMP_MID = 150
PUMP_HIGH = 255

// Timing
UPDATE_INTERVAL = 100ms
WATERING_INTERVAL = 24 hours (default)
```

### Customization

Settings are stored in EEPROM and can be modified through the menu system or by editing `config.h` for hardware-specific adjustments.

Happy Gardening! 🌿
