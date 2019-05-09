#include "BME280Sensor.h"
#include "Adafruit_CCS811.h"
#include "OLED.h"
#include <ArduinoLog.h>
#include <stdexcept>

BME280Sensor::BME280Sensor(QueueHandle_t readingsQueue) :
  Sensor (readingsQueue) {
}

int BME280Sensor::nReadings() {
  return 3;
}
void BME280Sensor::begin() {

  Sensor::begin();
  Log.trace("BME280Sensor::begin");

  int i;
  for (i = 0; i < 10 && !bme.begin(); i++) {
    delay(250);
  }

  if (i == 10) {
    throw std::runtime_error("Could not find BME280 sensor");
  }

  switch (bme.chipModel())  {
    case BME280::ChipModel_BME280:
      Log.verbose("BME280");
      break;
    case BME280::ChipModel_BMP280:
      Log.verbose("BMP280 without humidity.");
      break;
    default:
      throw std::runtime_error("BMXXXXX unknown sensor");
  }
  // calibrate? pressure?
  OLED.message("BME280Sensor:ok");

  readings[PRES]->metric = String("pressure");
  readings[HUM]->metric = String("humidity");
  readings[TEMP]->metric  = String("temperature");
  readings[PRES]->units = String("Pa");
  readings[HUM]->units = String("PC");
  readings[TEMP]->units  = String("C");
}

void BME280Sensor::read() {
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  float temp = NAN;
  float hum = NAN;
  float pres = NAN;

  Blob::i2cSemaphoreTake();
  bme.read(pres, temp, hum, tempUnit, presUnit);
  Blob::i2cSemaphoreGive();
  Log.verbose("BME280Sensor::read pres %.2f temp %.2f hum %.2f\n", pres, temp, hum);

  readings[PRES]->value = pres;
  readings[HUM]->value = hum;
  readings[TEMP]->value = temp;

}
