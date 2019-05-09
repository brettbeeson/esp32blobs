#pragma once

#include <Arduino.h>
#include "Sensor.h"

class Controller {

  public:
    Controller();
    virtual void begin() {};
    virtual void end() {};
    void taskify(int taskPeriodMs, int priority,int stackDepth=10000); // returns, spawns a task
    virtual void adjust()=0;
    bool showOnOLED = false;
    String toString();
    friend void ControllerAdjustReadTask(void *controllerPtr);
    int _taskPeriodMs=0;    
  private:
    
    
};
