#pragma once 

#include <freertos/FreeRTOS.h>
#include <Arduino.h>

void ReaderTask(void*);

class Reader {
  public:
    Reader(QueueHandle_t readingsQueue);
    virtual void taskify(int readFrequencyMs, int priority);
    virtual void read() = 0;
    
    friend class BlobBridge;
    friend class Blob;
    friend void ReaderTask(void *_p);
  protected:
    virtual void begin() = 0;
    QueueHandle_t _readingsQueue = NULL;
    int _readPeriodMs;
    
  private:

};
