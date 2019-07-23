#pragma once

#include <freertos/FreeRTOS.h>
#include <Arduino.h>
#include "Blob.h"

class Blob;

void PublisherTask(void*);

class Publisher {

  public:
    Publisher(Blob* blob);
    ~Publisher();
    virtual void begin() {};
    // Publish all readings in the queue. If block, never return. Publish individually. Override for batches.
    virtual int publishReadings(bool block);
    // Publish a single reading
    virtual bool publishReading(Reading *r) = 0;
    void taskify(int priority = 1);

    Blob* _blob = NULL;
    int _publishPeriodMs = 1000;
    QueueHandle_t _readingsQueue = NULL;

    eTaskState taskState();

    friend class Blob;

  protected:


  private:
    TaskHandle_t _publishTask;



};
