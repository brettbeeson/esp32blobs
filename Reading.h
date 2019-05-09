#pragma once 

#include <time.h>

/*
   Can be derivied. Maybe implement "MQTT Name" or similiar". Maybe store data in different types but ensure serializable.
*/
class Reading {
  public:

    Reading(const Reading &r);
    Reading();

    static const int elements  = 6;

    String metric;
    float value;            // 1
    String id;              // 2  ED^34324d
    String units;           // 3   C
    String location;        // 3  /home/DSTempSensorsss/
    time_t timestamp = 0;

    String tostr();
    String toJSON();
    String mqttTopic();
    String mqttValue();
    String valueStr(int decimals = 1, float minimum = NAN, float maximum = NAN);
    String shortStr();
    static Reading* fromJSON(String json);
    static Reading* fromJSON(const char* json);
    void clear();
};
