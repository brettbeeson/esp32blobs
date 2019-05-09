#pragma once

#include <freertos/FreeRTOS.h>
#include <Arduino.h>
#include "Blob.h"

void PublisherTask(void*);

class Publisher {

  public:
    Publisher(QueueHandle_t readingsQueue);
    void taskify(int _publishFrequencyMs, int priority);
    virtual void publish() = 0;
    virtual void begin() = 0;
    int _publishPeriodMs;
    QueueHandle_t _readingsQueue;

  protected:

  private:

};
