#pragma once

#include <Arduino.h>
#include "Sensor.h"
#include "Adafruit_CCS811.h"

class CCS811Sensor : public Sensor {
  public:
    CCS811Sensor(QueueHandle_t readingsQueue);
    void begin();
    void read();
    const byte ECO2 = 0;
    const byte TVOC = 1;
    const byte TEMP = 2;
    static void hardReset(int pin);   // CCS811 reset connected to this pin.
  
  private:
    Adafruit_CCS811 *ccs;
    float temp;
    int nReadings();
};
