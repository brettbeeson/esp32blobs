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


//class OledReadingsButton;
//class OledFramesButton;

class OLEDClass {
  public:
    OLEDClass ();
    ~OLEDClass();
    void begin (int sda=21, int scl=22,int displayControlPin=16,uint8_t address=0x3c);
    int refresh();
    void message(const String m);
    void off();

    CircularBuffer<String *, 5> _activeMessages; // need to make thread safe access? maybe use a stl queue
    
    long _lastReadingChangeMs = 0; // or via a press button
    
    void setTargetFPS(int fps);   // overrides OLEDDisplayUi targetFPS which I can't make work
    void setFramesToDisplay(uint8_t mask);
    void nextFrame();
    void nextReading();
    String _configMessage = "";
    static const uint8_t ConfigMask   = 1 << 0;
    static const uint8_t MessagesMask = 1 << 1;
    static const uint8_t ReadingsMask = 1 << 2;
    void addReading(Reading *r);
    void clearReadings();    
    
    void enableButtons();
    void disableButtons();
    void setButtons(int framesPin, int readingsPin, int threshold);

    CapButton *_framesButton;
    CapButton *_readingsButton;
    
    friend class OledReadingsButton;
    friend class OledFramesButton;
    friend void OLEDTask(void *OLEDPtr);
    
  private:
  
    vector<Reading *> _readings;
    size_t _iReading = 0;
    SSD1306 *_display;
    OLEDDisplayUi *_ui;
    long _lastFrameChangeMs=0;
    int _rst=0;
    
    static void drawFrameConfig(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
    static void drawFrameMessages(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
    static void drawFrameReadings(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
    static void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
    int framesCount();
    FrameCallback _frames[3];
    
    OverlayCallback _overlays[1];
    int overlaysCount();
    int _iFrame = 0;
    uint8_t _framesToDisplay=ConfigMask|MessagesMask|ReadingsMask;
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

extern OLEDClass OLED;
