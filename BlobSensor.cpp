#include <ArduinoLog.h>
#include "BlobSensor.h"

BlobSensor::BlobSensor(QueueHandle_t readingsQueue, VoltageMeasurement *vBatt) :
  _vBatt(vBatt),
  Sensor (readingsQueue) {
}

void BlobSensor::begin() {

  _nReadings = (_vBatt == NULL ? 2 : 3);

  Sensor::begin();

  readings[0]->metric = String("freeheap");
  readings[1]->metric = String("uptime");

  readings[0]->units = "B";
  readings[1]->units = "s";
  if (_vBatt != NULL) {
    readings[2]->metric = String("battery");
    readings[2]->units = "V";
    Log.verbose("vPin=%f vBatt=%f", _vBatt->vPin(), _vBatt->vTotal());
  }
}

void BlobSensor::read() {
  readings[0]->value = esp_get_free_heap_size();  //esp_get_minimum_free_heap_size() for contiguous;
  readings[1]->value = millis() / 1000; // will overflow!
  if (_vBatt != NULL) {
    readings[2]->value = _vBatt->vTotal();
  }
}

int BlobSensor::nReadings() {
  return _nReadings;
}
