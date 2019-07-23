#include <ArduinoLog.h>
#include "VoltageSensor.h"
#include "Blob.h"


VoltageSensor::VoltageSensor(Blob* blob, int pin, float r1, float r2, float cali)  :
  Sensor(blob), _pin(pin), _r1(r1), _rt(r1 + r2), _cali(cali) {
}


VoltageSensor ::~VoltageSensor () {
  for (int i = 0; i < nReadings(); i++) {
    if (this->readings[i]) delete(this->readings[i]); this->readings[i] = NULL;
  }
  this->readings = NULL;
}

void VoltageSensor::begin() {

  Sensor::begin();
  readings[0]->metric = String("battery");
  readings[0]->units = "V";
  readings[0]->id = _blob->id + "-battery";
  
}

void VoltageSensor::read() {
  float p,t;
  p = vPin();
  t = vTotal();
  //Serial.printf("vPin=%f vBatt=%f\n", p, t);
  readings[0]->setValue(t);
}


float VoltageSensor::vPin() {
  return ((float) analogRead(_pin) / 4096.0 * 3.3 * _cali);
}


float VoltageSensor::vTotal() {
   return vPin() * _rt /  _r1;
}
