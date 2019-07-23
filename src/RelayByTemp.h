#pragma once

#include <Arduino.h>
#include "Controller.h"
#include "DSTempSensors.h"

class RelayByTemp : public Controller {

  public:
    RelayByTemp(int pin, DSTempSensors* tempSensor);
    void begin();
    void adjust();
    ~RelayByTemp ();
  private:
    int _pin = 0;
    DSTempSensors* _tempSensor = NULL;
};
