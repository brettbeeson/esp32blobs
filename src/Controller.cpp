#include "Blob.h"
#include "Controller.h"
#include <stdexcept>
#include <RemoteDebug.h>

void ControllerAdjustTask(void *controllerPtr) {
  TickType_t xLastWakeTime;

  Controller* c = (Controller *) controllerPtr;
  // todo: wait until a multiple of the period
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(c->_taskPeriodMs));
    debugV("SensorReadTask");
    try {
      c->adjust();
    } catch (const std::runtime_error& e) {
      debugE("Failed SensorReadTask. Exception: %s",e.what());
    }
  }
}

Controller ::Controller () {
}

String Controller:: toString() {
  return String("Controller");
}

void Controller::taskify(int taskPeriodMs, int priority, int stackDepth) {
  _taskPeriodMs = taskPeriodMs;
  int core = xPortGetCoreID();
  debugV("Controller executing on core %d", core);
  xTaskCreatePinnedToCore(ControllerAdjustTask, "Controller", stackDepth, (void *) this, priority, NULL /* task handle */, core);
}
