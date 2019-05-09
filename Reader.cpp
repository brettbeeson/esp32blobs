#include "Reader.h"
#include "OLED.h"
#include <Arduino.h>
#include <ArduinoLog.h>

Reader::Reader(QueueHandle_t readingsQueue)
  : _readingsQueue(readingsQueue) {
}

void ReaderTask(void *_p)
{
  TickType_t xLastWakeTime;

  Reader* p = (Reader *) _p;
  
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(p->_readPeriodMs));
    try {
      p->read();
    } catch (const std::exception& e) {
      Log.error("%s\n",e.what());
      OLED.message(e.what());
    }
  }
}

void Reader::taskify(int readPeriodMs, int priority) {
  int core = 0;   // run on this core
  Log.verbose("Starting Reader task");
  _readPeriodMs = readPeriodMs;
  xTaskCreatePinnedToCore(ReaderTask, "Reader", 10000, (void *) this, priority, NULL /* no handle */, core);
}
