#include "sensors.h"

SensorManager::SensorManager() {
  lastMoistureReading = 0;
  lastWaterDetection = false;
  lastReadTime = 0;
}

void SensorManager::init() {
  pinMode(SOIL_POWER_PIN, OUTPUT);
  pinMode(WATER_DETECTION_POWER, OUTPUT);
  pinMode(WATER_SENSOR_PIN, INPUT);

  // Initialize sensors in off state
  powerSensors(false);
}

void SensorManager::updateReadings() {
  powerSensors(true);
  delay(SENSOR_WARM_TIME);

  lastMoistureReading = readSoilMoisture();
  lastWaterDetection = readWaterDetection();
  lastReadTime = millis();

  powerSensors(false);
}

int SensorManager::readSoilMoisture() {
  int reading = analogRead(SOIL_READ_PIN);
  delay(READ_DELAY);
  return reading;
}

bool SensorManager::readWaterDetection() {
  int reading = analogRead(WATER_DETECTION_READ);
  delay(READ_DELAY);
  return reading > WATER_DETECT_THRESHOLD;
}

int SensorManager::getMoistureLevel() { return lastMoistureReading; }

int SensorManager::getMoisturePercentage() {
  // Convert raw reading to percentage (0-100%)
  int percentage = map(lastMoistureReading, DRY_VALUE, WET_VALUE, 0, 100);
  return constrain(percentage, 0, 100);
}

bool SensorManager::isWaterDetected() { return lastWaterDetection; }

bool SensorManager::isSoilDry() {
  return lastMoistureReading <= DRY_VALUE + 50; // Add some hysteresis
}

bool SensorManager::isSoilWet() {
  return lastMoistureReading >= WET_VALUE - 50; // Add some hysteresis
}

SensorStatus SensorManager::getSoilStatus() {
  if (lastMoistureReading < 0 || lastMoistureReading > 1023) {
    return SENSOR_ERROR;
  }

  if (isSoilDry()) {
    return SENSOR_DRY;
  } else if (isSoilWet()) {
    return SENSOR_WET;
  }

  return SENSOR_OK;
}

SensorStatus SensorManager::getWaterStatus() {
  // Check if water sensor is working
  if (!digitalRead(WATER_SENSOR_PIN) && !lastWaterDetection) {
    return SENSOR_ERROR; // No water detected by either sensor - possible error
  }

  return SENSOR_OK;
}

bool SensorManager::sensorsHealthy() {
  return (getSoilStatus() != SENSOR_ERROR) &&
         (getWaterStatus() != SENSOR_ERROR);
}

void SensorManager::powerSensors(bool enable) {
  digitalWrite(SOIL_POWER_PIN, enable ? HIGH : LOW);
  digitalWrite(WATER_DETECTION_POWER, enable ? HIGH : LOW);
}

unsigned long SensorManager::getLastReadTime() const { return lastReadTime; }

void SensorManager::startCalibrationMode() { powerSensors(true); }

void SensorManager::endCalibrationMode() { powerSensors(false); }

int SensorManager::getRawMoistureReading() { return analogRead(SOIL_READ_PIN); }
