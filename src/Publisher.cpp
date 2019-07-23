
#define INCLUDE_vTaskSuspend                    1

#include "Publisher.h"
#include "Reading.h"
#include <Arduino.h>

void PublisherTask(void *_p)
{
  Publisher* p = (Publisher *) _p;

  for (;;) {
    p->publishReadings(true);
  }
}

Publisher::Publisher(Blob * blob):
  _blob(blob)
{
  blob->add(this);
}

Publisher::~Publisher() {
  if (_publishTask) {
    debugV("Stopping _publishTask\n");
    vTaskDelete(_publishTask);
  }
}


void Publisher::taskify(int priority) {
  int core = 0;   // run on this core
  debugV("Starting Publisher task in core %d\n", core);
  xTaskCreatePinnedToCore(PublisherTask, "Publisher", 10000, (void *) this, priority, &_publishTask /*no handle */, core);
}


eTaskState Publisher::taskState() {

  assert(_publishTask);
  return eTaskGetState(_publishTask);
}


/*
 * Publish all available
 */
int Publisher::publishReadings(bool block)
{
  BaseType_t xStatus = pdPASS;
  Reading* r = NULL;
  TickType_t xTicksToWait;
  int nPublished=0;

  xTicksToWait = (block ? portMAX_DELAY : 0);
  
  while (xStatus == pdPASS) {
    xStatus = xQueueReceive(_readingsQueue, &r, xTicksToWait);
    if (xStatus == pdPASS) {
      if (publishReading(r)) {
        nPublished++;
        if (r) delete r; r = NULL;
      } else {
        debugE("Couldn't publish reading!\n");
        if (r) delete r; r = NULL;
        // retain reading - should push_back
        // todo
      }
    } else {
      //debugV("Nothing to publish.block=%d xStatus = %d\n",block,xStatus);
    }
  }
  //if (nPublished) debugV("Published %d\n", nPublished);
  return nPublished;
}
