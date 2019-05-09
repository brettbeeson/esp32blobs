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

int OLEDClass::overlaysCount() {

  return sizeof(overlays) / sizeof(overlays[0]);
}

int OLEDClass::framesCount() {

  return sizeof(frames) / sizeof(frames[0]);
}


void OLEDClass::drawFrameConfig(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(0, 10, OLED.configMessage);
}


void OLEDClass::nextReading() {

  iReading++;
  if (iReading == readings.size()) iReading = 0;
}


void OLEDClass::drawFrameReadings(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);

  if (OLED.readings.size() == 0) {
    display->drawString(0, 10, "No readings");
  } else {
    if (!OLED.readingsButton && millis() - OLED.lastReadingChangeMs > 3000) {
      OLED.lastReadingChangeMs = millis();
      OLED.nextReading();
    }
    display->setFont(ArialMT_Plain_16);
    display->drawString(0, 10, OLED.readings[OLED.iReading]->metric);
    display->setFont(ArialMT_Plain_24);
    display->drawString(0, 26, String(OLED.readings[OLED.iReading]->value, 1));
    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    display->setFont(ArialMT_Plain_16);
    display->drawString(122, 31, OLED.readings[OLED.iReading]->units);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->setFont(ArialMT_Plain_10);
    display->drawString(0, 50, OLED.readings[OLED.iReading]->id);
  }
}

void OLEDClass::drawFrameMessages(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  for (int i = 0; i < OLED.activeMessages.size(); i++) {
    display->drawString(0, i * 10 + 10, *((String *) OLED.activeMessages[OLED.activeMessages.size() - i - 1]));
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
  readings.push_back(r);
}

void OLEDClass::clearReadings() {
  readings.clear();
}

OLEDClass::OLEDClass (int rst, int sda, int scl,  uint8_t address) :
  overlays{ msOverlay },
  frames { drawFrameConfig, drawFrameMessages, drawFrameReadings},
  lastFrameChangeMs(0),
  _rst(rst),
  display(address, sda, scl),  // this is I2C!
  ui(&display)
{
  Log.verbose("OLED: rst:%d sda:%d scl:%d addr:0x%04X frames:%d ", rst, sda, scl, address, framesCount());
  pinMode(LED_BUILTIN,OUTPUT);
}


void OLEDClass::begin () {

  pinMode(_rst, OUTPUT);
  digitalWrite(_rst, LOW);    // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(_rst, HIGH); // while OLED is running, must set GPIO16 in high
  ui.disableAllIndicators();
  ui.setFrames(frames, framesCount());
  ui.setTimePerTransition(0);
  ui.disableAutoTransition();
  ui.setOverlays(overlays, overlaysCount());

  Blob::i2cSemaphoreTake(); {
    Log.warning("Wire.begin() called\n");
    ui.init();       // does a  Wire.begin() And can fuck up i2c reads from BME280!
  } Blob::i2cSemaphoreGive();

  display.flipScreenVertically();
  
  int core = xPortGetCoreID();
  assert(core == 1);
  Log.trace("OLED::OLED Executing on core %d\n", core);
  xTaskCreatePinnedToCore(OLEDTask, "OLED", 10000 /*stack*/, (void *) this, 1 /* priority */, NULL /* return task handle */, core);
}

void OLEDClass::off() {
  
  display.displayOff();
  pinMode(_rst, OUTPUT);
  digitalWrite(_rst, LOW);    // set GPIO16 low to reset OLED
  /*
    display.sendCommand(0x8D); //into charger pump set mode
    display.sendCommand(0x10); //turn off charger pump
    display.sendCommand(0xAE); //set OLED sleep
  */
}

OLEDClass::~OLEDClass() {
  off();
  if (framesButton) delete readingsButton; framesButton = NULL;
  if (readingsButton) delete readingsButton; readingsButton = NULL;

}

void OLEDClass:: message(const String m) {
  // FIRST                LAST
  // newest               oldest
  //                      oldest is pop'd and removed
  //[,'b','c','d','e','f']
  // Copy the message to store in query. Caller can delete their instance
  // Delete when message is removed from queue
  String* queuedMessage = new String(m);
  if (activeMessages.isFull()) {
    String* oldie = activeMessages.pop(); // remove from last plaace
    delete(oldie);
  }
  activeMessages.unshift(queuedMessage); // put message into first place
}

void OLEDClass::setTargetFPS(int fps) {
  assert(fps > 1000/128); // huh? what? 128 is the max for int8_t, which is returned by update()! fucka
  ui.setTargetFPS(fps);
  
}
void OLEDClass::setFramesToDisplay(uint8_t mask) {
  framesToDisplay = mask;
  // move to first active
  while ( ! ((1 << iFrame) & framesToDisplay)) {
    if (++iFrame >= framesCount()) iFrame = 0;
  }
  ui.switchToFrame(iFrame);
}

/*
   Returns ms to next redraw to meet internal FPS
*/
int OLEDClass::refresh() {
  if (!framesButton && millis() - lastFrameChangeMs > 4000) {
    OLED.lastFrameChangeMs = millis();
    nextFrame();
  }
  return ui.update();
}

/**
   00000001
   00000010
   00000100
*/
void OLEDClass::nextFrame() {

  //DPRINTF("framesToDisplay=" BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(framesToDisplay));  DPRINTF(" framesCount=%d iFrame=%d\n",framesCount(),iFrame);
  if (!framesCount() || !framesToDisplay) return;
  if (++iFrame >= framesCount()) iFrame = 0;
  while ( ! ((1 << iFrame) & framesToDisplay)) {
    //DPRINTF("iFrame=%d, framesToDisplay=" BYTE_TO_BINARY_PATTERN"\n", iFrame,BYTE_TO_BINARY(framesToDisplay));
    if (++iFrame >= framesCount()) iFrame = 0;
  }
  ui.switchToFrame(iFrame);
}

void OLEDClass::disableButtons() {
  CapButton::pollingEnabled = false;
}

void OLEDClass::enableButtons() {
  CapButton::pollingEnabled = true;
}

void OLEDClass::setButtons(int framesPin, int readingsPin, int threshold = 40) {

  if (framesButton) delete framesButton;
  framesButton = new OledFramesButton(this, framesPin, threshold);
  framesButton->begin();

  if (readingsButton) delete readingsButton;
  readingsButton = new OledReadingsButton(this, readingsPin, threshold);
  readingsButton->begin();
}

OledReadingsButton::OledReadingsButton(OLEDClass *oled, int pin, int threshold ):
  CapButton(pin, threshold),
  _oled(oled)
{}

void OledReadingsButton::keypressEvent() {
  Log.verbose("OledReadingsButton KeyPress on pin %d\n", _pin);
  digitalWrite(LED_BUILTIN, 1);
  OLED.nextReading();
  _oled->ui.update();

}
void OledReadingsButton::keyupEvent() {
  Log.verbose("OledReadingsButton Keyup on pin %d\n", _pin);
  digitalWrite(LED_BUILTIN, 0);
}

/**
 * OledFramesButton
 */

OledFramesButton::OledFramesButton(OLEDClass *oled, int pin, int threshold) :
  CapButton(pin, threshold),
  _oled(oled)
{}

void OledFramesButton::keypressEvent() {
  Log.verbose("OledFramesButton KeyPress on pin %d\n", _pin);
  digitalWrite(LED_BUILTIN, 1);
  OLED.nextFrame();
  _oled->ui.update();

}
void OledFramesButton::keyupEvent() {
  Log.verbose("OledFramesButton Keyup on pin %d\n", _pin);
  digitalWrite(LED_BUILTIN, 0);
}
