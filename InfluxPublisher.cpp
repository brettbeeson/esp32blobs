#include "Blob.h"
//#include "OLED.h"
#include "InfluxPublisher.h"
#include "Reading.h"
#include <Arduino.h>
#include <WiFi.h>

InfluxPublisher:: InfluxPublisher(QueueHandle_t readingsQueue, String _server, int _port, String _db, String _measurement, String _user, String _password):
  Publisher(readingsQueue),
  influxdb(_server.c_str(), _port),
  db(_db),
  measurement(_measurement),
  user(_user),
  password(_password)
{
  Log.verbose("Influx::Influx:: %s,%d,%s,%s,%s\n", _server.c_str(), _port, db.c_str(), user.c_str(), password.c_str());
  influxdb._precision = "s";
}

void InfluxPublisher::begin() {
  if (!WiFi.status() == WL_CONNECTED)     throw std::runtime_error("InfluxPublisher::begin(): no Wifi");
  influxdb.opendb(db, user, password); // just stores data, no connection test.
  // test and throw if required
}

/*
  Empty the queue of Readings and publish in a batch
*/
void InfluxPublisher::publish() {
  BaseType_t xStatus = pdPASS;
  Reading* r = NULL;
  const TickType_t xTicksToWait = pdMS_TO_TICKS(10);
  int nPublished = 0;

  while (xStatus == pdPASS) {
    xStatus = xQueueReceive(_readingsQueue, &r, xTicksToWait);
    if (xStatus == pdPASS) {
      assert(r->id != ""); assert(r->location != ""); assert(r->metric != ""); assert(r->units != "");
      Log.verbose("Influxing: %s\n", r->tostr().c_str());
      dbMeasurement row(this->measurement);
      DB_RESPONSE retval;
      row.addTag("location", r->location);
      row.addTag("id", r->id);
      String f = r->metric + r->units;
      row.addField(f, r->value);
      if (r->timestamp) row.setTimeStamp(r->timestamp);  // seconds
      influxdb.addMeasurement(row); // copys
      nPublished++;
      if (r) delete r; r = NULL;
    }
  }

  int retval = influxdb.write();
  if (retval != DB_SUCCESS) {
    // readings are lost!
    String m = String("Influx failed:") + String(retval);
    throw std::runtime_error(m.c_str());
  }
  Log.verbose("Influxed: %d\n", nPublished);
  if (nPublished > 0) {
    
    //OLED.message(String("Influxed:") + String(nPublished));
  }
}
