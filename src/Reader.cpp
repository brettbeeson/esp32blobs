#include "Reader.h"
#include "Blob.h"
#include <Arduino.h>
#include <ArduinoLog.h>


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
      debugE("%s\n",e.what());
      //OLED.message(e.what());
    }
  }
}


Reader::Reader(Blob* blob)
  : _blob(blob) {
  
  blob->add(this);
}


void Reader::taskify(int readPeriodMs, int priority) {
  int core = 0;   // run on this core
  debugV("Starting Reader task\n");
  _readPeriodMs = readPeriodMs;
  xTaskCreatePinnedToCore(ReaderTask, "Reader", 10000, (void *) this, priority, NULL /* no handle */, core);
}
