#include "MQ3Sensor.h"
#include <ArduinoLog.h>

MQ3Sensor ::MQ3Sensor (QueueHandle_t readingsQueue, int pin)
  : Sensor(readingsQueue),
    _pin(pin) {
}

void MQ3Sensor ::begin() {
  Sensor::begin();
  readings[0]->metric = "alcohol";  
  readings[0]->units = "percent";  // percent of maximum
  readings[0]->id = "MQ3Alcohol";
  readings[0]->location = "air";
  pinMode(_pin, INPUT);
}

void MQ3Sensor ::read() {
  readings[0]->value = float(analogRead(_pin)) / 4096.0;
  //Log.verbose("MQ3Sensor rawRead:%d\n", analogRead(_pin) );
}

int MQ3Sensor::nReadings() {
  return 1;
}
