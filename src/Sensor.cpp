#include "freertos/FreeRTOS.h"
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include "Sensor.h"
#include "OLED.h"

void SensorReadTask(void *SensorSensor) {
  TickType_t xLastWakeTime;

  Sensor* s = (Sensor *) SensorSensor;
  // todo: wait until a multiple of the period
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(s->_taskPeriodMs));
    //debugV("SensorReadTask");
    try {
      s->read();
    } catch (const std::runtime_error& e) {
      debugE("Exception: %s", e.what());
    }
    s->copyReadingsToQueue();
  }
}

Sensor::Sensor(Blob *blob)
  :  _blob(blob), _taskPeriodMs(10000) {
    blob->add(this);
}

//todo: String location, String id
void Sensor::begin() {

  this->readings = (Reading **) malloc ((sizeof (Reading*)) * nReadings());
  for (int i = 0; i < nReadings(); i++) {
    this->readings[i] = new Reading();
    // for influx, it's important these are not empty. todo: omit in influx if empty.
    this->readings[i]->units = "";
    this->readings[i]->id = "";
    this->readings[i]->location = "";
    this->readings[i]->metric = "";
    //if (showOnOLED) OLED.addReading(this->readings[i]);
  }
}


void Sensor::setLocation(String location) {

  for (int i = 0; i < nReadings(); i++) {
    this->readings[i]->location = location;
  }
}


void Sensor::setID(String id) {

  for (int i = 0; i < nReadings(); i++) {
    this->readings[i]->id = id;
  }
}


String Sensor::toString() {
  String r;
  r = String("Sensor: nReadings=") + String(nReadings()) + String(": ");
  for (int i = 0; i < nReadings(); i++) {
    r += String("units:") + String(this->readings[i]->units) + String("; ");
    r += String("id:") + String(this->readings[i]->id) + String("; ");
    r += String("location:") + String(this->readings[i]->location) + String("; ");
    r += String("metric:") + String(this->readings[i]->metric) + String("; ");
  }
  return r;
}

void Sensor::taskify(int taskPeriodMs, int priority) {
  _taskPeriodMs = taskPeriodMs;
  int core = xPortGetCoreID();
  //debugV("Sensor executing on core %d", core);
  xTaskCreatePinnedToCore(SensorReadTask, "Sensor", 10000 /* Stack depth */, (void *) this, priority, NULL /* task handle */, core);
}

// write a copy to queue, keep lastest one in readings
void Sensor::copyReadingsToQueue() {
  BaseType_t xStatus;
  Reading* r = NULL;
  assert(_readingsQueue);
  for (int i = 0; i < nReadings(); i++) {
    r = new Reading(*(readings[i])); // copy it. receiver to delete
    xStatus = xQueueSendToBack(_readingsQueue, &r, pdMS_TO_TICKS(1000) /* timeout */);  // will copy the pointer
    if (xStatus == pdPASS) {
      // all good
    } else {
      delete(r); r = NULL;
      debugE("Could not send to the queue. Full = %d", xStatus == errQUEUE_FULL);

    }
  }
}

const Reading Sensor::getReading(int channel) {
  return (*getReadingPtr(channel));   // returns a copy
}

Reading* Sensor::getReadingPtr(int channel) {
  if (channel >= nReadings() || channel < 0) {
    throw std::range_error("No reading available on that channel");
  }
  return (readings[channel]);   // returns a ptr
}

void Sensor::clearReadings() {
  for (int i = 0; i < nReadings(); i++) {
    readings[i]->setValue(NAN);
    readings[i]->timestamp=0; 
  }
}

Sensor::~Sensor() {
  // 
}
