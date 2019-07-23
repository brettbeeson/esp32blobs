#include <ArduinoJson.h>
#include <RemoteDebug.h>
#include <Arduino.h>
#include "Reading.h"
#include <TimeKeeper.h>

extern RemoteDebug Debug;

#define SECONDS_TO_2000 8760*3600*30

Reading::Reading(const Reading &r) {
  value = r.value;
  units = r.units;
  location = r.location;
  metric = r.metric ;
  id = r.id ;
  timestamp = r.timestamp;
}

Reading::Reading(const measurement_t *m, const sample_t s) {

  value = s.v;
  timestamp = s.t;
  units = m->units;
  location = m->location;
  metric = m->metric;
  id = m->id;

}


Reading::Reading()   {}


float Reading:: getValue() {
  return value;
}


void Reading:: setValue(float v) {

  time_t t;


  //Serial.println(UTC.now());
  //Serial.println(TimeKeeper.now());
  //Serial.println(TimeKeeper.getPosix());

  // Not TimeKeeper.now() which gives time_t *in localtime* (bizarre)
  if ((t = UTC.now()) > SECONDS_TO_2000) {
    //debugV("Reading::setValue: t = %lu\n",UTC.now());
    timestamp = t;
  } else {
    debugV("Reading::setValue: no t\n");
    // pre-2000 : must be no date available
  }
  value = v;
}


void Reading:: setValue(float v, time_t t) {

  timestamp = t;
  value = v;
}

/**
 * todo// breaks on silly values for value
  // consider a char* or clip values
 */
String Reading::tostr() {
  
  
  return  String("Rdng::") + metric + "," + String(value, 2) + "," + id + "," + location + "," + units + "," + timestamp;
}


String Reading::toJSON() {
  String jsonStr;
  const size_t capacity = JSON_OBJECT_SIZE(Reading::elements) + 30;
  DynamicJsonDocument doc(capacity);
  
  doc["metric"] = metric;
  doc["value"] = value;
  doc["id"] = id;
  doc["location"] = location;
  doc["units"] = units;
  doc["timestamp"] = timestamp;
  serializeJson(doc, jsonStr);
  return jsonStr;
}

String Reading::shortStr() {
  // \todo Cache and make char[50] - called from OLED at 20Hz.
  String m;
  m = metric.substring(0, 5);
  m += "(";
  m += id.substring(0, 3);
  m += ")";
  m += "=";
  m += valueStr(1, -1000000, 1000000);
  return m;
}

String Reading::valueStr(int decimals, float minimum, float maximum) {
  float v = this->value;

  if (minimum != NAN) v = max(minimum, v);
  if (maximum != NAN) v = min(maximum, v);
  return String(v, decimals);
}

Reading* Reading ::fromJSON(String json) {

  Reading* r = NULL;
  const size_t capacity = JSON_OBJECT_SIZE(Reading::elements) + 30;
  DynamicJsonDocument jsonDoc(capacity);   // Static segfaults.
  
  DeserializationError error = deserializeJson(jsonDoc, json);
  if (error) {
    debugE("Failed to parse: %s\n", json.c_str());
  } else {
    r = new Reading();
    r->value = jsonDoc["value"];
    r->id = jsonDoc["id"].as<String>();       // \todo Is this the best way, with String? Or char* cpy?
    r->location = jsonDoc["location"].as<String>();
    r->metric = jsonDoc["metric"].as<String>();
    r->units = jsonDoc["units"].as<String>();
    r->timestamp = jsonDoc["timestamp"];   
  }
  return r;
}

Reading* Reading ::fromJSONBytes(uint8_t jsonBytes, int len) {
  char *s;
  String str;
  s = (char*) malloc(len); // sizeof(uint8_t) =assumed= 1
  memcpy((void*) s, (void*) jsonBytes, len);
  str = s;
  free(s); s = 0;
  return fromJSON(String(s));
}

uint8_t* Reading::toJSONBytes(int *len) {
  String r;
  char* s;
  r = this->toJSON();

  s = (char*) malloc(r.length()); // sizeof(uint8_t) =assumed= 1
  *len = r.length();
  strncpy(s, r.c_str(), *len);

  return (uint8_t*) s;
}


String Reading::mqttTopic() {
  return location + "/" + metric;
}
String Reading::mqttValue() {
  return String(value);

}
void Reading::clear() {
  value = NAN;
  //label = "";
  //units = "";
  //location = "";
}
