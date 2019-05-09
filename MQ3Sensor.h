#pragma once

#include <Arduino.h>
#include "Sensor.h"

class MQ3Sensor : public Sensor {

  public:
    MQ3Sensor(QueueHandle_t readingsQueue, int pin);
    void begin();
    void read();

  private:
    int _pin;
    float rawReading;
    float percentReading;
    int nReadings();

};
