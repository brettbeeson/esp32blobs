#pragma once
#include <time.h>
#include "Reading.h"

class Blob;

//void SensorReadTask(void*);

class Sensor {
  
  public:
    Sensor(Blob* blob);
    ~Sensor();
    virtual void begin();                              // starts hardware
    void end();
    void taskify(int sampleFrequencyMs, int priority); // returns, spawns a task
    virtual void read() = 0;                           // ask hardware to run, result stored in lastReadings
    void timestamp(time_t t);
    const Reading getReading(int channel);
    Reading* getReadingPtr(int channel);
    void copyReadingsToQueue();
    
    friend void SensorReadTask(void *SensorPtr);
    friend class Blob;
    void setLocation(String location);
    void setID(String id);
    String toString();

  protected:
    void clearReadings();
    virtual int nReadings() = 0;
    Reading** readings;    // Array of pointers. Sensor can have multiple readings (e.g. RH, temp).
    Blob *_blob;
    QueueHandle_t _readingsQueue;
    int _taskPeriodMs;
    
  private:
};
