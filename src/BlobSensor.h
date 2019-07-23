#pragma once
#include <Arduino.h>
#include "Sensor.h"


/**
 * Monitor internal parts of ESP32 (memory, uptime, etc.)
 */
class BlobSensor : public Sensor {
  public:
    BlobSensor(Blob* blob);
    void begin();
    void read();
    int nReadings();
  
  private:
    
    
    
};
