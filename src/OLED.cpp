#include "Blob.h"
#include "OLED.h"
#include "Reading.h"

#define LED_BUILTIN 25

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')


OLEDClass OLED; // global singleton

void OLEDTask(void *OLEDPtr) {

  TickType_t xLastWakeTime;
  int waitForMs;
  OLEDClass* p;
  p = (OLEDClass *) OLEDPtr;
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    Blob::i2cSemaphoreTake(); {
      waitForMs = p->refresh();
    } Blob::i2cSemaphoreGive();
    if (waitForMs > 0) {
      vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(waitForMs));
    }
  }
}


OLEDClass::OLEDClass () :
  _frames { drawFrameConfig, drawFrameMessages, drawFrameReadings},
  _overlays{ msOverlay }
{
  static int i = 0;
  assert(++i == 1); // singleton
  pinMode(LED_BUILTIN, OUTPUT);

}


void OLEDClass::begin (int rst, int sda, int scl,  uint8_t address) {

  debugV("OLED::begin rst:%d sda:%d scl:%d addr:0x%04X frames:%d ", rst, sda, scl, address, framesCount());

  _rst = rst;
  _display = new SSD1306(address, sda, scl);  // this is I2C!
  _ui = new OLEDDisplayUi (_display);

  pinMode(_rst, OUTPUT);
  digitalWrite(_rst, LOW);    // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(_rst, HIGH); // while OLED is running, must set GPIO16 in high
  _ui->disableAllIndicators();
  _ui->setFrames(_frames, framesCount());
  _ui->setTimePerTransition(0);
  _ui->disableAutoTransition();
  _ui->setOverlays(_overlays, overlaysCount());

  Blob::i2cSemaphoreTake(); {
    debugW("Wire.begin() called\n");
    _ui->init();       // does a  Wire.begin() And can fuck up i2c reads from BME280!
  } Blob::i2cSemaphoreGive();

  _display->flipScreenVertically();

  int core = xPortGetCoreID();
  assert(core == 1);
  debugV("OLED::OLED Executing on core %d\n", core);
  xTaskCreatePinnedToCore(OLEDTask, "OLED", 10000 /*stack*/, (void *) this, 1 /* priority */, NULL /* return task handle */, core);
}


int OLEDClass::overlaysCount() {

  return sizeof(_overlays) / sizeof(_overlays[0]);
}

int OLEDClass::framesCount() {

  return sizeof(_frames) / sizeof(_frames[0]);
}


void OLEDClass::drawFrameConfig(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(0, 10, OLED._configMessage);
}


void OLEDClass::nextReading() {

  _iReading++;
  if (_iReading == _readings.size()) _iReading = 0;
}


void OLEDClass::drawFrameReadings(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);

  if (OLED._readings.size() == 0) {
    display->drawString(0, 10, "No readings");
  } else {
    if (!OLED._readingsButton && millis() - OLED._lastReadingChangeMs > 3000) {
      OLED._lastReadingChangeMs = millis();
      OLED.nextReading();
    }
    display->setFont(ArialMT_Plain_16);
    display->drawString(0, 10, OLED._readings[OLED._iReading]->metric);
    display->setFont(ArialMT_Plain_24);
    display->drawString(0, 26, String(OLED._readings[OLED._iReading]->getValue(), 1));
    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    display->setFont(ArialMT_Plain_16);
    display->drawString(122, 31, OLED._readings[OLED._iReading]->units);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->setFont(ArialMT_Plain_10);
    display->drawString(0, 50, OLED._readings[OLED._iReading]->id);
  }
}

void OLEDClass::drawFrameMessages(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  for (int i = 0; i < OLED._activeMessages.size(); i++) {
    display->drawString(0, i * 10 + 10, *((String *) OLED._activeMessages[OLED._activeMessages.size() - i - 1]));
  }
}

void OLEDClass::msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {

  static int blip;

  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(126, 0, "H:" + String(esp_get_free_heap_size()));
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(20, 0, "U:" + String((int) (esp_timer_get_time() / 1000000) ));
  display->drawString(1, 0, (blip++ % 2 == 0) ? "X" : " ");
  display->drawLine(0, 11, 128, 11);
}

/*
   readings is an array of pointers owned by a Sensor so ... don't fuck with them.
   The caller should call clearReadings() before deleting them on their side
*/
void OLEDClass::addReading(Reading *r) {
  assert(r);
  _readings.push_back(r);
}

void OLEDClass::clearReadings() {
  _readings.clear();
}

void OLEDClass::off() {

  _display->displayOff();
  pinMode(_rst, OUTPUT);
  digitalWrite(_rst, LOW);    // set GPIO16 low to reset OLED
  /*
    _display->sendCommand(0x8D); //into charger pump set mode
    sendCommand(0x10); //turn off charger pump
    display.sendCommand(0xAE); //set OLED sleep
  */
}

OLEDClass::~OLEDClass() {
  off();
  if (_framesButton) delete _readingsButton; _framesButton = NULL;
  if (_readingsButton) delete _readingsButton; _readingsButton = NULL;

}

void OLEDClass:: message(const String m) {
  // FIRST                LAST
  // newest               oldest
  //                      oldest is pop'd and removed
  //[,'b','c','d','e','f']
  // Copy the message to store in query. Caller can delete their instance
  // Delete when message is removed from queue
  String* queuedMessage = new String(m);
  if (_activeMessages.isFull()) {
    String* oldie = _activeMessages.pop(); // remove from last plaace
    delete(oldie);
  }
  _activeMessages.unshift(queuedMessage); // put message into first place
}

void OLEDClass::setTargetFPS(int fps) {
  assert(fps > 1000 / 128); // huh? what? 128 is the max for int8_t, which is returned by update()! Tricky
  _ui->setTargetFPS(fps);

}
void OLEDClass::setFramesToDisplay(uint8_t mask) {
  _framesToDisplay = mask;
  // move to first active
  while ( ! ((1 << _iFrame) & _framesToDisplay)) {
    if (++_iFrame >= framesCount()) _iFrame = 0;
  }
  _ui->switchToFrame(_iFrame);
}

/*
   Returns ms to next redraw to meet internal FPS
*/
int OLEDClass::refresh() {
  if (!_framesButton && millis() - _lastFrameChangeMs > 4000) {
    // OLED.
    _lastFrameChangeMs = millis();
    nextFrame();
  }
  return _ui->update();
}

/**
   00000001
   00000010
   00000100
*/
void OLEDClass::nextFrame() {

  //DPRINTF("framesToDisplay=" BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(framesToDisplay));  DPRINTF(" framesCount=%d iFrame=%d\n",framesCount(),iFrame);
  if (!framesCount() || !_framesToDisplay) return;
  if (++_iFrame >= framesCount()) _iFrame = 0;
  while ( ! ((1 << _iFrame) & _framesToDisplay)) {
    //DPRINTF("iFrame=%d, framesToDisplay=" BYTE_TO_BINARY_PATTERN"\n", iFrame,BYTE_TO_BINARY(framesToDisplay));
    if (++_iFrame >= framesCount()) _iFrame = 0;
  }
  _ui->switchToFrame(_iFrame);
}

void OLEDClass::disableButtons() {
  CapButton::pollingEnabled = false;
}

void OLEDClass::enableButtons() {
  CapButton::pollingEnabled = true;
}

void OLEDClass::setButtons(int framesPin, int readingsPin, int threshold = 40) {

  if (_framesButton) delete _framesButton;
  _framesButton = new OledFramesButton(this, framesPin, threshold);
  _framesButton->begin();

  if (_readingsButton) delete _readingsButton;
  _readingsButton = new OledReadingsButton(this, readingsPin, threshold);
  _readingsButton->begin();
}

OledReadingsButton::OledReadingsButton(OLEDClass *oled, int pin, int threshold ):
  CapButton(pin, threshold),
  _oled(oled)
{}

void OledReadingsButton::keypressEvent() {
  debugV("OledReadingsButton KeyPress on pin %d\n", _pin);
  digitalWrite(LED_BUILTIN, HIGH);
  OLED.nextReading();
  _oled->_ui->update();

}
void OledReadingsButton::keyupEvent() {
  debugV("OledReadingsButton Keyup on pin %d\n", _pin);
  digitalWrite(LED_BUILTIN, LOW);
}

/**
   OledFramesButton
*/

OledFramesButton::OledFramesButton(OLEDClass *oled, int pin, int threshold) :
  CapButton(pin, threshold),
  _oled(oled)
{}

void OledFramesButton::keypressEvent() {
  debugV("OledFramesButton KeyPress on pin %d\n", _pin);
  digitalWrite(LED_BUILTIN, 1);
  OLED.nextFrame();
  _oled->_ui->update();

}
void OledFramesButton::keyupEvent() {
  debugV("OledFramesButton Keyup on pin %d\n", _pin);
  digitalWrite(LED_BUILTIN, 0);
}
