#pragma once
#include <rom/rtc.h>
#include <freertos/FreeRTOS.h>
#include <Arduino.h>
#include <RemoteDebug.h>

#include "LoRa.h"
#include <SPI.h>
#include <vector>

extern RemoteDebug Debug;

using namespace std;

String resetReasonStr(RESET_REASON reason);

class Controller;
class Publisher;
class Sensor;
class Reader;
class Reading;
class BlobWebServer;

class Blob {
  
  public:
  
    Blob();
    ~Blob();
    virtual void begin(int useServicesMask);
    virtual void begin();

    // Publish current readings and return
    virtual void publish();

    void addReading(Reading *r, int iReadingsQueue = 0);
    void testBasicFunctions();
    bool configFromJSON(Stream &s);
    void readSensors();
    void readReaders();
    void adjustControllers();

    String toString();

    void printReadingsQueue(int i = 0);
    QueueHandle_t readingsQueue(int i);
    UBaseType_t readingsInQueue(int i=0);

    String location = "/blob/default/location/";
    String id;
    
    static void i2cSemaphoreTake();
    static void i2cSemaphoreGive();

    void add(Publisher *p);
    void add(Reader *r);
    void add(Controller *c);
    void add(Sensor *s);

    void remove(Publisher *p);
    void remove(Reader *r);
    void remove(Controller *c);
    void remove(Sensor *s);

    static const unsigned int USE_I2C =  1 << 0;
    static const unsigned int USE_SD =   1 << 1;
    static const unsigned int USE_WIFI =  1 << 2;
    static const unsigned int USE_LORA = 1 << 3;
    static const unsigned int USE_HTTPD = 1 << 4;

    int wifiAPWaitMs=0;

  protected:

    Publisher *publisher=NULL;
    Reader* _reader=NULL;
    vector<Sensor *> sensors;
    vector<Controller *> controllers;
    

    static String getMacStr();

    void setupVoltageMeasurement();

  private:
    BlobWebServer* webserver=NULL;
    int useServicesMask;
    static int scanI2C();
    bool testSDCard();
    bool testLora();
    static SemaphoreHandle_t i2cSemaphore;

    QueueHandle_t _readingsQueue0;
    QueueHandle_t _readingsQueue1;

    void setupI2CBus();
    void setupSDCard();
    void setupLora();
    void setupWebServer();
    // Setup global Wifi. Throw if cannot connect. Wait forever (default), or waitS seconds for the config AP portal.
    void setupWiFi();




};
