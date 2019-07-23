#include <WiFiManager.h>          // https://github.com/Brunez3BD/WIFIMANAGER-ESP32
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <SPIFFS.h>
#include <WiFi.h>
#include <Wire.h>
#include <Arduino.h>
#include <SD.h>
#include <exception>
//#include <rom/rtc.h>

#include "LoRa.h"
#include "Blob.h"
#include "Reading.h"
#include "Publisher.h"
#include "Controller.h"
#include "Reader.h"
#include "BlobWebServer.h"
#include "Sensor.h"

using namespace std;

SemaphoreHandle_t Blob::i2cSemaphore = NULL;

void BlobWifiConfigModeCallback(WiFiManager *myWiFiManager) {
  debugI("Entered wifi mode at %s:%s\n", WiFi.softAPIP().c_str(), myWiFiManager->getConfigPortalSSID().c_str());
  //OLED.message("No WiFi. Connect to A:P");
  //OLED.message(myWiFiManager->getConfigPortalSSID());
  //OLED.message(WiFi.softAPIP().toString());
}

Blob::Blob() 
  
{
  i2cSemaphore = xSemaphoreCreateMutex();
  assert (i2cSemaphore);
  _readingsQueue0 = xQueueCreate( 30, sizeof(Reading *));
  _readingsQueue1 = xQueueCreate( 30, sizeof(Reading *));
  assert(_readingsQueue0); assert(_readingsQueue1);
  id = getMacStr();
}


void Blob::add(Publisher *p) {

  // segfaults \todo check why
  //if (_publisher) delete publisher; publisher = 0;
  publisher = p;
  publisher->_readingsQueue = this->readingsQueue(0);
  //debugV("Blob::addPublisher readingsQueue0=%d\n", publisher->_readingsQueue);
}

void Blob::remove(Publisher *p) {

  assert(publisher == p);
  delete publisher; publisher = 0;
}

void Blob::add(Reader *r) {

  if (_reader) delete _reader; _reader = 0;
  _reader = r;
  _reader ->_readingsQueue = this->readingsQueue(0);
}


/*
   Add reading to queue. When used, it will be deleted. Caller: don't delete.
*/
void Blob::addReading(Reading *r, int iReadingsQueue) {
  BaseType_t xStatus ;
  assert(r);
  xStatus = xQueueSendToBack(this->readingsQueue(iReadingsQueue), &r, pdMS_TO_TICKS(100) /* timeout */);
  if (xStatus != pdPASS) {
    delete(r); r = NULL; // todo: maybe don't?
    debugE("%s: xQueueSendToBack fail with status = %d", __FUNCTION__, xStatus);
    throw runtime_error("addReading: xQueueSendToBack fail");
  }

}


void Blob::begin() {

  begin(0);
}


void Blob::begin(int useServicesMask) {

  Reading *r;

  if (useServicesMask & USE_I2C)  setupI2CBus();
  if (useServicesMask & USE_SD)   setupSDCard();
  if (useServicesMask & USE_WIFI) setupWiFi();
  if (useServicesMask & USE_LORA) setupLora();
  if (useServicesMask & USE_HTTPD) setupWebServer();

  for (std::size_t i = 0; i < sensors.size(); ++i)  {
    sensors[i]->begin();
      if (r = sensors[i]->getReadingPtr(0)) {
        if (r->location == "") sensors[i]->setLocation(location);
      }
      if (r = sensors[i]->getReadingPtr(0)) {
        if (r->id == "") sensors[i]->setID(id);
      }
    }
  if (publisher) publisher->begin();
  if (_reader) _reader->begin();
  for (std::size_t i = 0; i < controllers.size(); ++i)  {
    controllers[i]->begin();
    }
}


void Blob::setupLora() {

  SPIClass *spiLora;

  spiLora = new SPIClass();
  LoRa.setSPI(*spiLora);
  //       SCLK , MISO , MOSI , SS
  spiLora->begin(5, 19, 27, 18);
  //void setPins(int ss = LORA_DEFAULT_SS_PIN, int reset = LORA_DEFAULT_RESET_PIN, int dio0 = LORA_DEFAULT_DIO0_PIN);
  LoRa.setPins(18, 23, 26);
  if (!LoRa.begin(915E6)) {
    throw std::runtime_error("Couldn't start Lora");
  }
  LoRa.setSyncWord(0xBB); // ranges from 0-0xFF, default 0x34, see API docs
  LoRa.enableCrc();
}


void Blob::setupSDCard() {

  int sdCS;
  SPIClass *spiSD;

  assert(0); // not working yet
  sdCS = 13; //SS
  spiSD = new SPIClass();
  //SCLK , MISO , MOSI , SS
  spiSD->begin(14, 2, 15, sdCS);
  //bool begin(uint8_t ssPin=SS, SPIClass &spi=SPI, uint32_t frequency=4000000, const char * mountpoint="/sd")
  Serial.println(sdCS);
  if (!SD.begin(sdCS, *spiSD)) throw std::runtime_error("Couldn't start SD");
}


void Blob::setupWiFi() {
  WiFiManager wifiManager;

  //wifiManager.resetSettings(); for testing
  wifiManager.setDebugOutput(false); // todo use logger value
  wifiManager.setAPCallback(BlobWifiConfigModeCallback);
  if (wifiAPWaitMs<0) {
    // infinite
  } else if (wifiAPWaitMs > 0) {
    wifiManager.setConfigPortalTimeout(wifiAPWaitMs);
  } else { // ==0
    wifiManager.setConfigPortalTimeout(0);
  }

  if (!wifiManager.autoConnect("BlobAP")) {
    throw std::runtime_error("Failed to connect and hit timeout\n");
  }
  debugI("Connected to Wifi %s\n", wifiManager.getSSID().c_str());
}


void Blob::setupWebServer() {

  bool retval;

  webserver = new BlobWebServer(80);
  retval = webserver ->setup();
  if (!retval) throw std::runtime_error("Couldn't setup Webserver");
}

void Blob::setupI2CBus() {

  int nDevices;
  int restartResult;

  i2cSemaphoreTake(); {
    debugW("Wire.begin() called");
    debugV("I2C setup on SDA=%d SCL=%d\n", SDA, SCL);
    Wire.begin(); // Pins 21,22 (SDA,SCL);
    nDevices = scanI2C();
    if (nDevices == 0 /*!=expectedI2CDevices*/) {
      Serial.print("Warning: I2C scan found no devices. Reseting bus.");
      assert(0);
      //restartResult = restartI2CBus();
      debugI("I2C reset result was: %d\n", restartResult);
      Wire.begin(); // Pins 21,22 (SDA,SCL);
      nDevices = scanI2C();
      if (nDevices == 0) {
        //throw std::runtime_error("I2C scan didn't find devices");
      }
    }
    debugI("I2C scan found %d devices.\n", nDevices);
  }  i2cSemaphoreGive();

}


bool Blob::configFromJSON(Stream &s) {
  return false;  // not implemented. read a JSON string and configure myself
}

void Blob::i2cSemaphoreTake() {
  xSemaphoreTake(i2cSemaphore, portMAX_DELAY);
}

void Blob::i2cSemaphoreGive() {
  xSemaphoreGive(i2cSemaphore);
}


/*
 * Publish current readings and return
 */
void Blob::publish() {
  
  if (publisher) publisher->publishReadings(false);
}


void Blob::testBasicFunctions() {
  
  if (!testLora()) throw std::runtime_error("Lora Check Failed");
  if (!testSDCard()) throw std::runtime_error("SD Check Failed");
}


bool Blob::testSDCard() {
  
  return SD.exists("/index.htm");
}


bool Blob::testLora() {
  
  bool retval;
  retval = LoRa.beginPacket();
  retval = retval && (LoRa.write(1) == 1);
  retval = retval && LoRa.endPacket();
  return retval;
}


/**
 * Warning: careful not to call subclass methods
 */
Blob::~Blob() {
  // Caller to delete sensors, etc, as they are usually local variable
  // 
  //LoRa.end();
  
  //WiFi.disconnect();

  // free i2cSemaphore?
/*
 * if (webserver) delete webserver; webserver = NULL;
  for (std::size_t i = 0; i < sensors.size(); ++i)  {
    if (sensors[i]) delete(sensors[i]); sensors[i] = 0;
  }
  for (std::size_t i = 0; i < controllers.size(); ++i)  {
    if (controllers[i]) delete(controllers[i]); controllers[i] = 0;
  }
  if (publisher) delete(publisher); publisher = 0;
  if (_readerpublisher) delete(publisher); publisher = 0;
  //if (WIRE_HAS_END)  Wire.end();
  */
}


String Blob::getMacStr() {
  const uint8_t MACLEN = 6;
  uint8_t mac[MACLEN];
  String s;

  memset(mac, 0, MACLEN );
  ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac));
  for (int i = 0; i < MACLEN ; i++) {
    if (s != "") s += ".";
    s += String(mac[i]);
  }

  return s;
}

void Blob::add(Controller *c) {
  controllers.push_back(c);
}


void Blob::add(Sensor *s) {
  s->_readingsQueue = this->readingsQueue(0);
  sensors.push_back(s);
}


void Blob::readSensors() {
  //debugV("Blob::readSensors() size:%d\n",sensors.size());
  for (std::size_t i = 0; i < sensors.size(); ++i)  {
    
    sensors[i]->read();
    sensors[i]->copyReadingsToQueue();
  }
}

void Blob::readReaders() {
  if (_reader) {
    _reader->read();
  }
}

void Blob::adjustControllers() {
  for (std::size_t i = 0; i < controllers.size(); ++i)  {
    controllers[i]->adjust();
  }
}


String Blob::toString() {
  String r;
  for (std::size_t i = 0; i < controllers.size(); ++i)  {
    r += controllers[i]->toString();
  }
  for (std::size_t i = 0; i < sensors.size(); ++i)  {
    r += sensors[i]->toString();
  }
//  if (_reader) r+= _reader->toString();
//  if (publisher) r+= publisher->toString();
  return r;
}


String resetReasonStr(RESET_REASON reason) {
  switch ( reason)
  {
    case 1 : return String ("POWERON_RESET"); break;         /**<1, Vbat power on reset*/
    case 3 : return String("SW_RESET"); break;              /**<3, Software reset digital core*/
    case 4 : return String ("OWDT_RESET"); break;            /**<4, Legacy watch dog reset digital core*/
    case 5 : return String ("DEEPSLEEP_RESET"); break;       /**<5, Deep Sleep reset digital core*/
    case 6 : return String("SDIO_RESET"); break;            /**<6, Reset by SLC module, reset digital core*/
    case 7 : return String ("TG0WDT_SYS_RESET"); break;      /**<7, Timer Group0 Watch dog reset digital core*/
    case 8 : return String ("TG1WDT_SYS_RESET"); break;      /**<8, Timer Group1 Watch dog reset digital core*/
    case 9 : return String ("RTCWDT_SYS_RESET"); break;      /**<9, RTC Watch dog Reset digital core*/
    case 10 : return String ("INTRUSION_RESET"); break;      /**<10, Instrusion tested to reset CPU*/
    case 11 : return String ("TGWDT_CPU_RESET"); break;      /**<11, Time Group reset CPU*/
    case 12 : return String ("SW_CPU_RESET"); break;         /**<12, Software reset CPU*/
    case 13 : return String ("RTCWDT_CPU_RESET"); break;     /**<13, RTC Watch dog Reset CPU*/
    case 14 : return String ("EXT_CPU_RESET"); break;        /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : return String ("RTCWDT_BROWN_OUT_RESET"); break; /**<15, Reset when the vdd voltage is not stable*/
    case 16 : return String ("RTCWDT_RTC_RESET"); break;     /**<16, RTC Watch dog reset digital core and rtc module*/
    default : return String ("NO_MEAN");
  }
}


void Blob::printReadingsQueue(int i) {
  debugI("readingsQueue%d: %d items\n", i, uxQueueMessagesWaiting(this->readingsQueue(i)));
}

QueueHandle_t Blob::readingsQueue(int i) {
  if (i == 0) {
    return _readingsQueue0;
  } else if (i == 1) {
    return _readingsQueue1;
  } else {
    assert(0);
  }
}

UBaseType_t Blob::readingsInQueue(int i) {
  return uxQueueMessagesWaiting( readingsQueue(i) );
}

int Blob::scanI2C() {
  byte error, address;
  int  nDevices = 0;
  for (address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      debugI("I2C device found at address 0x (not implemented)\n");
      /*    if (address < 16)
            debugI("0");
          debugI(address, HEX);
          debugI("  !");
      */
      nDevices++;
    }
    else if (error == 4)
    {
      debugE("Unknown error at address 0x");
      /*
        if (address < 16)
        debugE("0");
        debugE(address, HEX);
      */
    }
  }
  if (nDevices == 0) {
    debugI("No I2C devices found\n");
  }
  return nDevices;
}
