#include "CapButton.h"
#include "Blob.h"

int CapButton::nInstance = 0;
CapButton* CapButton::instances[5] = {0, 0, 0, 0, 0};
int CapButton::pollPeriodMs = 10;
unsigned long CapButton::debounceDelay = 25;    // the debounce time; increase if the output flickers
bool CapButton::pollingEnabled = true;

void CapButton::PollAllTask(void *) {
  TickType_t xLastWakeTime;
  CapButton::printHooks();
  xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CapButton::pollPeriodMs));
    if (pollingEnabled) {
      for (int i = 0; i < MAX_INSTANCES && CapButton::instances[i]; i++) {
        CapButton::instances[i]->pollTask();
      }
    }
  }
}

CapButton::CapButton(int pin, int threshold , bool interrupts ):
  _pin(pin) ,
  _threshold(threshold),
  _interrupts(interrupts) {
  _nthInstance = nInstance;
  if (_nthInstance < MAX_INSTANCES) {
    instances[_nthInstance] = this;
  }
  nInstance++;
}

void CapButton::pollTask() {

  bool readingNow;
  int readingNowRaw; 
  readingNowRaw = touchRead(_pin);
  readingNow = readingNowRaw < _threshold;
  //
// Serial.printf("pin %d\t reading=%d \n", _pin, readingNowRaw); Serial.flush();
  if (readingNow != _lastState) {
    _lastDebounceTime = millis();
  //  Serial.printf("change on pin %d reading=%d \n", _pin, readingNowRaw); Serial.flush();
     
  }

  if ((millis() - _lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:
    if (readingNow != _currentState) {
      // the button state has changed
      _currentState = readingNow;
      if (_currentState) {
        keypressEvent();
      } else {
        keyupEvent();
      }
    } else {
      // the button state is the same
      // Could implement "still down" for repetative LLLLLLLLLLLLLLLLetters
    }
  }
  _lastState = readingNow;
}

void CapButton::begin() {
  if (_interrupts) {
    switch (_nthInstance) {
      case 0:
        touchAttachInterrupt(_pin, touchCallBack0, _threshold);
        break;
      case 1:
        touchAttachInterrupt(_pin, touchCallBack1, _threshold);
        break;
      case 2:
        touchAttachInterrupt(_pin, touchCallBack2, _threshold);
        break;
      case 3:
        touchAttachInterrupt(_pin, touchCallBack3, _threshold);
        break;
      case 4:
        touchAttachInterrupt(_pin, touchCallBack4, _threshold);
        break;
      default:
        Serial.println("Crikey, that's too many CapButtons!");
        //throw std::out_of_range("Crikey, that's too many CapButtons!");
        break;
    }
  }

  static bool taskStarted = false;
  if (!taskStarted) {
    Log.verbose("Starting CapButtonPollTask");
    int core = 1;   // run on this core, as the OLED does
    xTaskCreatePinnedToCore(CapButton::PollAllTask, "CapButtonPollTask", 10000, (CapButton *) instances, priority, NULL /* no handle */, core);
  }
  taskStarted = true;;
}

void CapButton ::printHooks() {
  CapButton *cb;
  for (int i = 0; i < 5; i++) {
    if (instances[i] ) {
      Serial.printf("instance[%d].pin = %d this=%d n=%d\n", i, instances[i]->_pin, instances[i], instances[i]->_nthInstance);
    }
  }
}

/*
   Sample implementation
*/
ButtonPrinter::ButtonPrinter(int pin, int threshold , bool interrupts ):
  CapButton(pin, threshold, interrupts) {}

void ButtonPrinter::keypressEvent() {
  Serial.printf("KeyPress on pin %d\n", _pin);
}
void ButtonPrinter::keyupEvent() {
  Serial.printf("Keyup on pin %d\n", _pin);
}
