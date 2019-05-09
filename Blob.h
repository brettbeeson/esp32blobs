#pragma once
#include <rom/rtc.h>
#include <freertos/FreeRTOS.h>
#include <Arduino.h>
#include <ArduinoLog.h>

#include "LoRa.h"
#include <SPI.h>
#include "BlobWebServer.h"
#include "Publisher.h"
#include "Reading.h"
#include "Controller.h"
#include "Sensor.h"
#include "Reader.h"

#include <vector>

class Controller;

using namespace std;

String resetReasonStr(RESET_REASON reason);

class Publisher;
class Sensor;
class Reader;

class Blob {
  public:
    Blob();
    ~Blob();
    virtual void begin();
    virtual void end();
    virtual void publish();
    void addPublisher(Publisher *p);
    void addReader(Reader *r);
    void addReading(Reading *r);
    void addController(Controller *c);
    void addSensor(Sensor *s);
    void testBasicFunctions();
    bool configFromJSON(Stream &s);
    void readSensors();
    void adjustControllers();

    String toString();

    // Should ask all components to flag using bit mask and autostart required elements. Manual for now.
    void setupI2CBus();
    void setupSDCard();
    void setupLora();
    void setupWebServer();
    void setupWiFi();
    
    void printReadingsQueue();
    
    String location = "/blob/default/location/";
    String id;
    void sleep(int ms);
    void deepSleep(int ms);

    QueueHandle_t readingsQueue;
    static void i2cSemaphoreTake();
    static void i2cSemaphoreGive();
    friend class LoraReader;
    
  protected:
    int restartI2CBus();
    Publisher *publisher;
    vector<Sensor *> sensors;
    vector<Controller *> controllers;
    Reader* _reader;
    static String getMacStr();
    void setupVoltageMeasurement();
    
  private:
    BlobWebServer* webserver;
    SPIClass *spiLora;
    SPIClass *spiSD;
    static int scanI2C();
    bool testSDCard();
    bool testLora();
    static SemaphoreHandle_t i2cSemaphore;
    int readingsQueueLength = 50;
};
