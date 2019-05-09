#include <ArduinoJson.h>
#include <ArduinoLog.h>

#include <Arduino.h>
#include "Reading.h"

Reading::Reading(const Reading &r) {
  value = r.value;
  units = r.units;
  location = r.location;
  metric = r.metric ;
  id = r.id ;
}

Reading::Reading() :
  value(NAN) {
}

String Reading::tostr() {
  // breaks on silly values for value
  // consider a char* or clip values
  return  String("Rdng::") + metric + "," + String(value, 2) + "," + id + "," + location + "," + units + "," + timestamp;
}
/*
String Reading::hasWhitespace() {
  return 
}
*/

String Reading::toJSON() {
  String retval;
  //const int capacity = JSON_OBJECT_SIZE(Reading::elements);
  //StaticJsonBuffer<capacity> jsonBuffer;
  const size_t capacity = JSON_OBJECT_SIZE(Reading::elements) + 30;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject&obj = jsonBuffer.createObject();
  obj["metric"] = metric;
  obj["value"] = value;
  obj["id"] = id;
  obj["location"] = location;
  obj["units"] = units;
  obj["timestamp"] = timestamp;
  obj.printTo(retval);
  return retval;
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
  DynamicJsonBuffer jsonBuffer(capacity);   // Static segfaults.

  JsonObject& obj = jsonBuffer.parseObject(json);
  if (obj.success()) {
    r = new Reading();
    r->value = obj["value"];
    r->id = obj["id"].as<String>();       // \todo Is this the best way, with String? Or char* cpy?
    r->location = obj["location"].as<String>();
    r->metric = obj["metric"].as<String>();
    r->units = obj["units"].as<String>();
    r->timestamp = obj["timestamp"];
  } else {
    Log.error("Failed to parse: %s\n",json.c_str());
  }
  return r;
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
