#pragma once
#include <Arduino.h>
#include "Sensor.h"

class VoltageMeasurement;

class BlobSensor : public Sensor {
  public:
    BlobSensor(QueueHandle_t readingsQueue,VoltageMeasurement *vBatt=NULL);
    void begin();
    void read();
    int nReadings();
  
  private:
    VoltageMeasurement *_vBatt;
    int _nReadings;
    
};

//
//   vTotal -|      <--- connected voltage to read (>3.3v)
//           | r2
//   vPin ---|      <--- pin 
//           | r1
//   gnd-----|      
//
class VoltageMeasurement {
  public:
    
    VoltageMeasurement(int pin, float r1, float r2, float cali = 1.0) :
      _pin(pin), _r1(r1), _rt(r1 + r2), _cali(cali) {
    }
    float vPin() {      
      return ((float) analogRead(_pin) / 4096.0 * 3.3 * _cali);
    }
    float vTotal() {
      return vPin() * _rt /  _r1;
    }
private:
    int _pin;
    float _r1, _rt, _cali;
};
