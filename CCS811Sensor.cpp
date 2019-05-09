#include "Blob.h"
#include "OLED.h"
#include "CCS811Sensor.h"

CCS811Sensor::CCS811Sensor(QueueHandle_t readingsQueue) :
  Sensor (readingsQueue) {
}
void CCS811Sensor::hardReset(int pin) {
  Log.notice("Forcing I2C CCS811 reset of with pin %d\n", pin);
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);       // reset via pull down
  delay(100);
  digitalWrite(pin, HIGH);        // ready
  delay(250);
}

void CCS811Sensor::begin() {
  Sensor::begin();

  readings[ECO2]->units = "ppm";
  readings[TVOC]->units = "ppb";
  readings[TEMP]->units = "C";
  readings[ECO2]->metric = "eco2";
  readings[TVOC]->metric = "tvoc";
  readings[TEMP]->metric = "temp";

  this->ccs = new Adafruit_CCS811();

  if (!this->ccs->begin()) {
    Log.error("Failed to start CO2 sensor! Please check your wiring.");
    //OLED.message("CCS811Sensor fail");
    while (1);
  }

  while (!this->ccs->available()) {
    delay(500);
    Log.trace("Waiting for CCS811Sensor");
    //OLED.message("Waiting for CCS811Sensor");
  }
  //OLED.message("CCS811Sensor ok");
  this->temp = this->ccs->calculateTemperature();
  this->temp = 25;
  Log.verbose("CCS811Sensor::temp=%.2f", temp);
  if ( (this->temp) < -100 || (this->temp) > 100) {
    throw std::runtime_error("CCS811Sensor:: invalid temperature");
  }
}

void CCS811Sensor::read() {
  int retval;

  Sensor::clearReadings();

  if (ccs->available()) {
    temp = ccs->calculateTemperature();
    temp = 25;  // forced \todo
    if (! (retval = ccs->readData())) {
      readings[ECO2]->value = ccs->geteCO2();
      readings[TVOC]->value = ccs->getTVOC();
      readings[TEMP]->value = temp;
    } else {
      throw std::runtime_error("Could not readData() CCS811 sensor");
    }
    Log.verbose("CCS811Sensor::readHardware eco2 %.2f temp %.2f tvoc %.2f", readings[ECO2]->value, temp, readings[TVOC]->value);
  } else {
    throw std::runtime_error("Could not available() CCS811 sensor");
  }
}

int CCS811Sensor::nReadings() {
  return 3;
}
