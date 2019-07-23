#include <inttypes.h>
#include <soc/rtc.h>
#include "Reading.h"
#include "BlobSensor.h"
#include "Blob.h"
#include "MemorySaver.h"
#include "MemoryReader.h"
#include "InfluxPublisher.h"
#include "TimeKeeper.h"
#include "VoltageSensor.h"
#include "DSTempSensors.h"
#include "BBEsp32Lib.h"

#include <RemoteDebug.h>




#define TIME_TO_SLEEP_USEC 60000000
#define BULK_SEND_X 10

const int VoltageSensorPin = 36;
const int TempSensorPin = 22;

#define TRACE Serial.println(String(__FILE__) + " Line: " +  String(__LINE__));

TimeKeeperClass TimeKeeper("AEST-10");
RemoteDebug Debug;

void setup() {

  Serial.begin(115200); Serial.flush();
  
  Debug.setSerialEnabled(true);   // Print to serial too
  
  TimeKeeper.restoreTime();

  ezt::setServer("au.pool.ntp.org");

  Serial.printf("%s",TimeKeeper.toString().c_str());

  try {
    if ( TimeKeeper.getBootCount()  == 1) {
      // On hard reset clear and arrange pointers in RTC memory
      MemorySaver::init();
    }

    if ( TimeKeeper.getBootCount()  == 1 || TimeKeeper.getBootCount() % BULK_SEND_X == 0) {
      // On reset and every nth wakeup, connect to WiFi, set time and upload data (if any)
      //Debug.begin("timelapse", RemoteDebug::VERBOSE); // Initialize the WiFi server
      Blob wifiBlob;
      InfluxPublisher p(&wifiBlob, "monitor.phisaver.com", 8086, "test", "batt", "bbeeson", "imagine");
      MemoryReader r(&wifiBlob);
      wifiBlob.begin(Blob::USE_WIFI);
      // setDebug(INFO);
      // todo: test if this fails. retry for a bit.
      TimeKeeper.updateFromNTP();
      debugV("New NTP time: %s\n", TimeKeeper.dateTime().c_str());
      debugV("Influxing %d samples\n", r.nTotalSamples());
      // Taskify Reader ...
      r.taskify(100 /* period */, 1 /* priority */); // read from memory
      // ... and run publisher until it's read all the samples
      while (r.nTotalSamples() > 0) {
        // publish, don't block
        p.publishReadings(false);
        debugV("Queue: %d Memory: %d \n", wifiBlob.readingsInQueue(), r.nTotalSamples());
        delay(100);
      }
      // ... then run publisher until it is idling
      while (p.publishReadings(false));

      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);

      //  assert(!WiFi.isConnected()); fails - maybe need to wait for other CPU?
    }
  } catch (std:: exception &e) {
    debugV("%s",e.what());
  }

  try {
    // For all wakeups, read sensors and save to RTC memory
    Blob sensorBlob;
    BlobSensor bs(&sensorBlob);
    VoltageSensor vs(&sensorBlob, VoltageSensorPin, 1600000, 1600000, 1.0);
    DSTempSensors ts(&sensorBlob, TempSensorPin);
    MemorySaver saveToMemory(&sensorBlob);

    sensorBlob.begin();
    sensorBlob.readSensors(); // read from sensors
    sensorBlob.publish(); // save to RTC and return
    //debugI("Done publish to Memory: Measurements: %d Samples: %d\n", saveToMemory.nMeasurements(), saveToMemory.nTotalSamples());
  } catch (std:: exception &e) {
    debugV("%s",e.what());
  }
  Serial.flush();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_USEC); // - _overSleepUSec);
  esp_deep_sleep_start();
  assert(0);
}

void loop() {
  // put your main code here, to run repeatedly:

}
