#pragma once

#include "Reading.h"

//void SensorReadTask(void*);

class Sensor {
  
  public:
    Sensor(QueueHandle_t _readingsQueue);
    ~Sensor();
    virtual void begin();                              // starts hardware
    void end();
    void taskify(int sampleFrequencyMs, int priority); // returns, spawns a task
    virtual void read() = 0;                           // ask hardware to run, result stored in lastReadings
    const Reading getReading(int channel);
    Reading* getReadingPtr(int channel);
    void copyReadingsToQueue();
    bool showOnOLED = false;
    friend void SensorReadTask(void *SensorPtr);
    void setLocation(String location);
    void setID(String id);
    String toString();

  protected:
    void clearReadings();
    virtual int nReadings() = 0;
    Reading** readings;    // Array of pointers. Sensor can have multiple readings (e.g. RH, temp).
    QueueHandle_t _readingsQueue;
    int _taskPeriodMs;
    
  private:
};
