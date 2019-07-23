#include "MQ3Sensor.h"
#include "Blob.h"
#include <ArduinoLog.h>

MQ3Sensor ::MQ3Sensor (Blob* blob, int pin)
  : Sensor(blob), _pin(pin) {
}

void MQ3Sensor ::begin() {
  Sensor::begin();
  readings[0]->metric = "alcohol";
  readings[0]->units = "percent";  // percent of maximum
  readings[0]->id = _blob->id + "-MQ3Alcohol";
  readings[0]->location = "air";
  pinMode(_pin, INPUT);
}

MQ3Sensor ::~MQ3Sensor () {
  for (int i = 0; i < nReadings(); i++) {
    if (this->readings[i]) delete(this->readings[i]); this->readings[i] = NULL;
  }
  this->readings = NULL;
}


void MQ3Sensor ::read() {
  readings[0]->setValue ( float(analogRead(_pin)) / 4096.0);
  //debugV("MQ3Sensor rawRead:%d\n", analogRead(_pin) );
}

int MQ3Sensor::nReadings() {
  return 1;
}
