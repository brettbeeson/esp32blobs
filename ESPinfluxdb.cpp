#include "Arduino.h"
#include "ESPinfluxdb.h"

//#define DEBUG_PRINT // comment this line to disable debug print

#ifndef DEBUG_PRINT
#define DEBUG_PRINT(a)
#else
#define DEBUG_PRINT(a) (Serial.println(String(F("[Debug]: "))+(a)))
#define _DEBUG
#endif

Influxdb::Influxdb(const char *host, uint16_t port) {
  _port = String(port);
  _host = String(host);
}

DB_RESPONSE Influxdb::opendb(String db, String user, String password) {
  _db = db + "&u=" + user + "&p=" + password;
}

DB_RESPONSE Influxdb::opendb(String db) {

  HTTPClient http;
  http.begin("http://" + _host + ":" + _port + "/query?q=show%20databases"); //HTTP

  int httpCode = http.GET();

  if (httpCode == 200) {
    _response = DB_SUCCESS;
    String payload = http.getString();
    http.end();

    if (payload.indexOf("db") > 0) {
      _db =  db;
      Serial.println(payload);
      return _response;
    }
  } else {
    Serial.println(HTTPClient::errorToString(httpCode));
  }
  _response = DB_ERROR;
  DEBUG_PRINT("Database open failed");
  return _response;

}

void Influxdb::addMeasurement(dbMeasurement data) {
  measurements.push_back(data);
}


DB_RESPONSE Influxdb::write() {
  HTTPClient http;
  String data;
  int httpResponseCode;
  int tries = 0;
  DEBUG_PRINT("HTTP post begin...");

  http.begin("http://" + _host + ":" + _port + "/write?db=" + _db + "&precision=" + _precision); //HTTP
  http.addHeader("Content-Type", "text/plain");
  DEBUG_PRINT("http://" + _host + ":" + _port + "/write?db=" + _db + "&precision=" + _precision);

  for (size_t i; i < measurements.size(); i++) {
    if (i > 0) data += "\n";
    data += measurements[i].postString();
  }
  DEBUG_PRINT(data);
  httpResponseCode = -1;
  while (httpResponseCode == -1) {  // while "connection refused", just keep trying a few times
    httpResponseCode = http.POST(data);
    if (tries) {
      String m = String("Influxdb write failed ") + String(tries) + String (" times");
      Serial.println(m);
      delay(1000 * tries);    // back off tex
    }
    tries++;
    if (tries>3) {
      return DB_CONNECT_FAILED;
    }
  }
  if (httpResponseCode == 204) {
    _response = DB_SUCCESS;
    String response = http.getString();    //Get the response to the request
    DEBUG_PRINT(String(httpResponseCode)); //Print return code
    DEBUG_PRINT(response);                 //Print request answer
  } else {
    Serial.printf("Influx error on sending POST. httpResponseCode=%d error=%s\n", httpResponseCode, String(HTTPClient::errorToString(httpResponseCode)).c_str());
    _response = DB_ERROR;
  }
  measurements.clear();
  http.end();
  return _response;
}
DB_RESPONSE Influxdb::write(dbMeasurement data) {
  return write(data.postString());
}

DB_RESPONSE Influxdb::write(String data) {


  HTTPClient http;

  DEBUG_PRINT("HTTP post begin...");

  http.begin("http://" + _host + ":" + _port + "/write?db=" + _db + "&precision=" + _precision); //HTTP
  http.addHeader("Content-Type", "text/plain");

  DEBUG_PRINT("http://" + _host + ":" + _port + "/write?db=" + _db + "&precision=" + _precision);
  DEBUG_PRINT(data);
  int httpResponseCode = http.POST(data);

  if (httpResponseCode == 204) {
    _response = DB_SUCCESS;
    String response = http.getString();    //Get the response to the request
    DEBUG_PRINT(String(httpResponseCode)); //Print return code
    DEBUG_PRINT(response);                 //Print request answer
  } else {
    Serial.printf("Influx error on sending POST. httpResponseCode=%d\n", httpResponseCode);
    _response = DB_ERROR;
  }

  http.end();
  return _response;
}

DB_RESPONSE Influxdb::query(String sql) {

  String url = "/query?";
  url += "pretty=true&";
  url += "db=" + _db;
  url += "&q=" + URLEncode(sql);
  DEBUG_PRINT("Requesting URL: ");
  DEBUG_PRINT(url);

  HTTPClient http;

  http.begin("http://" + _host + ":" + _port + url); //HTTP


  // start connection and send HTTP header
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode == 200) {
    // HTTP header has been send and Server response header has been handled
    _response = DB_SUCCESS;
    String reply = http.getString();
    Serial.println(reply);

  } else {
    _response = DB_ERROR;
    DEBUG_PRINT("[HTTP] GET... failed, error: " + httpCode);
  }

  http.end();
  return _response;
}


DB_RESPONSE Influxdb::response() {
  return _response;
}

/* -----------------------------------------------*/
//        Field object
/* -----------------------------------------------*/
dbMeasurement::dbMeasurement(String m) {
  measurement = m;
}

void dbMeasurement::empty() {
  _data = "";
  _tag = "";
  _timestamp = "";
}

void dbMeasurement::addTag(String key, String value) {
  _tag += "," + key + "=" + value;
}

void dbMeasurement::addField(String key, float value) {
  _data = (_data == "") ? (" ") : (_data += ",");
  _data += key + "=" + String(value);
}

// only second precisino allowed for
void dbMeasurement::setTimeStamp(time_t tsUnixEpochSeconds) {
  // \todo consider _precision!
  _timestamp = " " + String(tsUnixEpochSeconds);
}

String dbMeasurement::postString() {
  return measurement + _tag + _data + _timestamp;
}

// URL Encode with Arduino String object
String URLEncode(String msg) {
  const char *hex = "0123456789abcdef";
  String encodedMsg = "";

  uint16_t i;
  for (i = 0; i < msg.length(); i++) {
    if (('a' <= msg.charAt(i) && msg.charAt(i) <= 'z') ||
        ('A' <= msg.charAt(i) && msg.charAt(i) <= 'Z') ||
        ('0' <= msg.charAt(i) && msg.charAt(i) <= '9')) {
      encodedMsg += msg.charAt(i);
    } else {
      encodedMsg += '%';
      encodedMsg += hex[msg.charAt(i) >> 4];
      encodedMsg += hex[msg.charAt(i) & 15];
    }
  }
  return encodedMsg;
}
