#include "Blob.h"
#include "Reading.h"
#include <WiFiManager.h>          // https://github.com/Brunez3BD/WIFIMANAGER-ESP32
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <SPIFFS.h>
#include <WiFi.h>
#include <Wire.h>
#include "LoRa.h"
#include <Arduino.h>
#include <SD.h>
#include <exception>
#include <rom/rtc.h>

using namespace std;

SemaphoreHandle_t Blob::i2cSemaphore = NULL;


void BlobWifiConfigModeCallback(WiFiManager *myWiFiManager) {
  Log.notice("Entered wifi mode at %s:%s\n", WiFi.softAPIP(), myWiFiManager->getConfigPortalSSID());
  //OLED.message("No WiFi. Connect to A:P");
  //OLED.message(myWiFiManager->getConfigPortalSSID());
  //OLED.message(WiFi.softAPIP().toString());
}

Blob::Blob() :
  publisher(NULL),
  webserver(NULL),
  spiLora(NULL),
  spiSD(NULL)
{
  i2cSemaphore = xSemaphoreCreateMutex();
  assert (i2cSemaphore);
  readingsQueue = xQueueCreate( readingsQueueLength, sizeof(Reading *));
  assert(readingsQueue);
  id = getMacStr();
  Log.verbose("Blob::getMacStr = %s\n", id.c_str());
}


// \todo MOdify LoRa.h to take an instance of SPIClass
void Blob::setupLora() {
  spiLora = new SPIClass();
  LoRa.setSPI(*spiLora);
  //SCLK , MISO , MOSI , SS
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

  assert(0);
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
  wifiManager.setConfigPortalTimeout(60);  // A

  if (!wifiManager.autoConnect("BlobAP")) {
    throw std::runtime_error("Failed to connect and hit timeout\n");
  }
  Log.notice("Connected to Wifi %s\n", wifiManager.getSSID().c_str());
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
    Log.warning("Wire.begin() called");
    Log.verbose("I2C setup on SDA=%d SCL=%d\n", SDA, SCL);
    Wire.begin(); // Pins 21,22 (SDA,SCL);
    nDevices = scanI2C();
    if (nDevices == 0 /*!=expectedI2CDevices*/) {
      Serial.print("Warning: I2C scan found no devices. Reseting bus.");
      restartResult = restartI2CBus();
      Log.notice("I2C reset result was: %d\n", restartResult);
      Wire.begin(); // Pins 21,22 (SDA,SCL);
      nDevices = scanI2C();
      if (nDevices == 0) {
        //throw std::runtime_error("I2C scan didn't find devices");
      }
    }
    Log.notice("I2C scan found %d devices.\n", nDevices);
  }  i2cSemaphoreGive();

}

void Blob::addPublisher(Publisher *p) {
  // segfaults \todo check why
  //if (publisher) delete publisher; publisher = 0;
  publisher = p;
  Log.verbose("Blob::addPublisher readingsQueue=%d\n", p->_readingsQueue);
}

void Blob::addReader(Reader *r) {
  //if (_reader) delete _reader; _reader = 0;
  assert(readingsQueue); assert(this);
  //r->readingsQueue = this->readingsQueue;
  //r->blob = this;
  _reader = r;
}


/*
   Add reading to queue. When used, it will be deleted. Caller: don't delete.
*/
void Blob::addReading(Reading *r) {
  BaseType_t xStatus ;
  assert(r);
  xStatus = xQueueSendToBack(this->readingsQueue, &r, pdMS_TO_TICKS(1000) /* timeout */);
  if (xStatus != pdPASS) {
    delete(r); r = NULL; // todo: maybe don't?
    Log.error("%s: xQueueSendToBack fail with status = %d", __FUNCTION__, xStatus);
    throw runtime_error("addReading: xQueueSendToBack fail");
  }
  
}

void Blob::begin() {
  Reading *r;
  for (std::size_t i = 0; i < sensors.size(); ++i)  {
    sensors[i]->begin();
    if (r = sensors[i]->getReadingPtr(0)) {
      if (r->location == "") sensors[i]->setLocation(location);
    }
    if (r = sensors[i]->getReadingPtr(0)) {
      if (r->id=="") sensors[i]->setID(id);
    }
  }
  if (publisher) publisher->begin();
  for (std::size_t i = 0; i < controllers.size(); ++i)  {
    controllers[i]->begin();
  }
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


void Blob::publish() {
  if (publisher) publisher->publish();
}

void Blob::testBasicFunctions() {
  assert(spiLora && spiSD);
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
      Log.notice("I2C device found at address 0x (not implemented)\n");
      /*    if (address < 16)
            Log.notice("0");
          Log.notice(address, HEX);
          Log.notice("  !");
      */
      nDevices++;
    }
    else if (error == 4)
    {
      Log.error("Unknown error at address 0x");
      /*
        if (address < 16)
        Log.error("0");
        Log.error(address, HEX);
      */
    }
  }
  if (nDevices == 0) {
    Log.notice("No I2C devices found\n");
  }
  return nDevices;
}

int Blob::restartI2CBus() {
  /**
    I2C_ClearBus
    (http://www.forward.com.au/pfod/ArduinoProgramming/I2C_ClearBus/index.html)
    (c)2014 Forward Computing and Control Pty. Ltd.
    NSW Australia, www.forward.com.au
    This code may be freely used for both private and commerical use
  */

  /**
     This routine turns off the I2C bus and clears it
     on return SCA and SCL pins are tri-state inputs.
     You need to call Wire.begin() after this to re-enable I2C
     This routine does NOT use the Wire library at all.

     returns 0 if bus cleared
             1 if SCL held low.
             2 if SDA held low by slave clock stretch for > 2sec
             3 if SDA held low after 20 clocks.
  */

#if defined(TWCR) && defined(TWEN)
  TWCR &= ~(_BV(TWEN)); //Disable the Atmel 2-Wire interface so we can control the SDA and SCL pins directly
#endif

  pinMode(SDA, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
  pinMode(SCL, INPUT_PULLUP);

  delay(2500);  // Wait 2.5 secs. This is strictly only necessary on the first power
  // up of the DS3231 module to allow it to initialize properly,
  // but is also assists in reliable programming of FioV3 boards as it gives the
  // IDE a chance to start uploaded the program
  // before existing sketch confuses the IDE by sending Serial data.

  boolean SCL_LOW = (digitalRead(SCL) == LOW); // Check is SCL is Low.
  if (SCL_LOW) { //If it is held low Arduno cannot become the I2C master.
    return 1; //I2C bus error. Could not clear SCL clock line held low
  }

  boolean SDA_LOW = (digitalRead(SDA) == LOW);  // vi. Check SDA input.
  int clockCount = 20; // > 2x9 clock

  while (SDA_LOW && (clockCount > 0)) { //  vii. If SDA is Low,
    clockCount--;
    // Note: I2C bus is open collector so do NOT drive SCL or SDA high.
    pinMode(SCL, INPUT); // release SCL pullup so that when made output it will be LOW
    pinMode(SCL, OUTPUT); // then clock SCL Low
    delayMicroseconds(10); //  for >5uS
    pinMode(SCL, INPUT); // release SCL LOW
    pinMode(SCL, INPUT_PULLUP); // turn on pullup resistors again
    // do not force high as slave may be holding it low for clock stretching.
    delayMicroseconds(10); //  for >5uS
    // The >5uS is so that even the slowest I2C devices are handled.
    SCL_LOW = (digitalRead(SCL) == LOW); // Check if SCL is Low.
    int counter = 20;
    while (SCL_LOW && (counter > 0)) {  //  loop waiting for SCL to become High only wait 2sec.
      counter--;
      delay(100);
      SCL_LOW = (digitalRead(SCL) == LOW);
    }
    if (SCL_LOW) { // still low after 2 sec error
      return 2; // I2C bus error. Could not clear. SCL clock line held low by slave clock stretch for >2sec
    }
    SDA_LOW = (digitalRead(SDA) == LOW); //   and check SDA input again and loop
  }
  if (SDA_LOW) { // still low
    return 3; // I2C bus error. Could not clear. SDA data line held low
  }

  // else pull SDA line low for Start or Repeated Start
  pinMode(SDA, INPUT); // remove pullup.
  pinMode(SDA, OUTPUT);  // and then make it LOW i.e. send an I2C Start or Repeated start control.
  // When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
  /// A Repeat Start is a Start occurring after a Start with no intervening Stop.
  delayMicroseconds(10); // wait >5uS
  pinMode(SDA, INPUT); // remove output low
  pinMode(SDA, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.
  delayMicroseconds(10); // x. wait >5uS
  pinMode(SDA, INPUT); // and reset pins as tri-state inputs which is the default state on reset
  pinMode(SCL, INPUT);
  return 0; // all ok
}


Blob::~Blob() {
  this->end();
}

void Blob::sleep(int ms) {
  delay(ms);
}

void Blob::deepSleep(int ms) {
  end();
  esp_sleep_enable_timer_wakeup(ms * 1000  ); // microscends
  esp_deep_sleep_start();
  // restarts at setup()
  assert(0);
}


void Blob::end() {
  LoRa.end();
  if (webserver) delete webserver; webserver = NULL;
  if (spiLora) {
    spiLora->end();
    delete spiLora;
    spiLora = NULL;
  }
  if (spiSD) {
    spiSD->end();
    delete spiSD;
    spiSD = NULL;
  }
  WiFi.disconnect();
  // free i2cSemaphore?
  
  // a destructor, but called from deepSleep
  for (std::size_t i = 0; i < sensors.size(); ++i)  {
    if (sensors[i]) delete(sensors[i]); sensors[i] = 0;
  }
  for (std::size_t i = 0; i < controllers.size(); ++i)  {
    if (controllers[i]) delete(controllers[i]); controllers[i] = 0;
  }

  if (publisher) delete(publisher); publisher = 0;
  //if (WIRE_HAS_END)  Wire.end();
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

void Blob::addController(Controller *c) {
  controllers.push_back(c);
}

void Blob::addSensor(Sensor *s) {
  sensors.push_back(s);
}
void Blob::readSensors() {
  for (std::size_t i = 0; i < sensors.size(); ++i)  {
    sensors[i]->read();
    sensors[i]->copyReadingsToQueue();
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


void Blob::printReadingsQueue() {
  Serial.print("Blob::readingsQueue has ");
  Serial.print(uxQueueMessagesWaiting(readingsQueue ));
  Serial.print(" items\n");
}
