/*
   LoraPublisher.h

    Created on: 8 Jan 2019
        Author: bbeeson
*/

#ifndef LoraPublisher_H_
#define LoraPublisher_H_

#include "Blob.h"
#include "Publisher.h"

void LoraPublisherTask(void*);

class LoraPublisher : public Publisher {
  public:
    LoraPublisher(QueueHandle_t readingsQueue);
    void publish();
    void begin();
  private:

};

#endif
