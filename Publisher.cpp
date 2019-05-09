#include "Publisher.h"
#include <Arduino.h>

void PublisherTask(void *_p)
{
  TickType_t xLastWakeTime;

  Publisher* p = (Publisher *) _p;
  // todo: wait until a multiple of the period
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(p->_publishPeriodMs));
    try {
      p->publish();
    } catch (const std::exception& e) {
      Log.error("%s\n",e.what());
      //OLED.message(e.what());
    }
  }
}

Publisher::Publisher(QueueHandle_t readingsQueue):
  _readingsQueue(readingsQueue), _publishPeriodMs(1000)
{}

void Publisher::taskify(int publishPeriodMs, int priority) {
  int core = 0;   // run on this core
  Log.verbose("Starting Publisher task");
  _publishPeriodMs = publishPeriodMs;
  xTaskCreatePinnedToCore(PublisherTask, "Publisher", 10000, (void *) this, priority, NULL /* no handle */, core);
}
