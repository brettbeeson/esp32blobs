#pragma once

#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include "OLEDDisplayUi.h"
#include "Blob.h"
#include "Reading.h"
#include <SPI.h>
#include <CircularBuffer.h>
#include "CapButton.h"
#include <vector>

using namespace std;

class OLEDClass;
extern OLEDClass OLED;

//class OledReadingsButton;
//class OledFramesButton;

class OLEDClass {
  public:
    OLEDClass (int sda=21, int scl=22,int displayControlPin=16,uint8_t address=0x3c);  // Only create one!
    ~OLEDClass();
    void begin ();
    int refresh();
    void message(const String m);
    void off();

    CircularBuffer<String *, 5> activeMessages; // need to make thread safe access? maybe use a stl queue
    
    long lastReadingChangeMs = 0; // or via a press button
    void setTargetFPS(int fps);   // overrides OLEDDisplayUi targetFPS which I can't make work
    void setFramesToDisplay(uint8_t mask);
    void nextFrame();
    void nextReading();
    String configMessage = "";
    static const uint8_t ConfigMask   = 1 << 0;
    static const uint8_t MessagesMask = 1 << 1;
    static const uint8_t ReadingsMask = 1 << 2;
    void addReading(Reading *r);
    void clearReadings();    
    
    void enableButtons();
    void disableButtons();
    void setButtons(int framesPin, int readingsPin, int threshold);

    CapButton *framesButton;
    CapButton *readingsButton;
    
    friend class OledReadingsButton;
    friend class OledFramesButton;
    friend void OLEDTask(void *OLEDPtr);
    
  private:
  
    vector<Reading *> readings;
    size_t iReading = 0;
    SSD1306 display;
    OLEDDisplayUi ui;
    long lastFrameChangeMs;
    int _rst;
    //int _lastRefreshMs;
    static void drawFrameConfig(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
    static void drawFrameMessages(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
    static void drawFrameReadings(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
    static void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
    
    OverlayCallback overlays[1];
    FrameCallback frames[3];
    int overlaysCount();
    int framesCount();

    int iFrame = 0;
    uint8_t framesToDisplay=ConfigMask|MessagesMask|ReadingsMask;
};

class OledFramesButton : public CapButton {
  public:
    OledFramesButton(OLEDClass* oled, int pin, int threshold = 40);
    void keypressEvent();
    void keyupEvent() ;
  private:
    OLEDClass *_oled;
};

class OledReadingsButton : public CapButton {
  public:
    OledReadingsButton(OLEDClass* oled, int pin, int threshold = 40);
    void keypressEvent();
    void keyupEvent();
  private:
    OLEDClass *_oled;
};
