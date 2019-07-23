#include <ArduinoLog.h>
#include "BlobSensor.h"
#include "Blob.h"

BlobSensor::BlobSensor(Blob* blob) :
  Sensor (blob) {
}


void BlobSensor::begin() {
  Sensor::begin();
  assert(_blob);
  readings[0]->metric = String("freeheap");
  readings[0]->units = "B";
  readings[0]->id = _blob->id + "-" + readings[0]->metric;
  readings[1]->metric = String("uptime");
  readings[1]->units = "s";
  readings[1]->id = _blob->id + "-" + readings[1]->metric ;

}

void BlobSensor::read() {
  readings[0]->setValue(esp_get_free_heap_size());  // esp_get_minimum_free_heap_size() for contiguous;
  readings[1]->setValue(millis() / 1000);         // will overflow!
}


int BlobSensor::nReadings() {

  return 2;
}
