#include "TimeTask.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include "time.h"
#include <WiFi.h>
#include <exception>

TimeTaskClass TimeTask;

TimeTaskClass::TimeTaskClass() {}

TimeTaskClass::~TimeTaskClass() {}

/*
  init and get the time
   runs sntp server in background
*/
void TimeTaskClass::begin(long  _gmtOffset_sec, int   _daylightOffset_sec, const char* _ntpServer ) {
  gmtOffset_sec = _gmtOffset_sec;
  daylightOffset_sec = _daylightOffset_sec;
  ntpServer = _ntpServer;
  if (!WiFi.status() == WL_CONNECTED)     throw std::runtime_error("TimeTaskClass::refresh(): no Wifi");
  assert(ntpServer);
  Serial.printf("TimeTaskClass::begin: os=%d dl=%d ntp=%s\n", gmtOffset_sec, daylightOffset_sec, ntpServer);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  waitForSet();
  refresh();
}

/*
   init and get the time
   runs sntp server in background
   segfaults!
*/

void TimeTaskClass::begin(const char* _tz, const char* _ntpServer ) {
  
  tz = _tz;
  ntpServer = _ntpServer;
  if (!WiFi.status() == WL_CONNECTED)throw std::runtime_error("TimeTaskClass::refresh(): no Wifi");
  assert(ntpServer); assert(tz);
  Serial.printf("TimeTaskClass::begin: tz=%s ntp=%s\n", gmtOffset_sec, daylightOffset_sec, ntpServer);
  configTzTime(tz, ntpServer); //, const char* server2, const char* server3)
  waitForSet();
  refresh();
}

void TimeTaskClass::waitForSet(int msToWait) {
  struct tm timeinfo ;
  if (getLocalTime(&timeinfo, msToWait)) {  // wait up to 10sec to sync
    Serial.println(&timeinfo, "Time set: %B %d %Y %H:%M:%S (%A)");
  } else {
    Serial.printf("Cound't set time using %s\n", ntpServer);
  }
  Serial.flush();
}

void TimeTaskClass::refresh() {
  lastTimeCheck = time(NULL);
  //Serial.printf("TimeTaskClass::refresh() n=%d\n", lastTimeCheck);
  assert(lastTimeCheck != -1);
}

const char* TimeTaskClass::getLocalTimeStr() {
  static char s[50];
  strftime(s, sizeof(s), "%Y-%m-%dT%H:%M:%SZ", localtime(&lastTimeCheck));
  return s;
}

const char* TimeTaskClass::getUTCTimeStr() {
  static char s[50];
  strftime(s, sizeof(s), "%Y-%m-%dT%H:%M:%SZ", gmtime(&lastTimeCheck));
  return s;
}


const char* TimeTaskClass::getLocalTimeStr(time_t t) {
  static char s[50];
  strftime(s, sizeof(s), "%Y-%m-%dT%H:%M:%SZ", localtime(&t));
  return s;
}

const char* TimeTaskClass::getUTCTimeStr(time_t t) {
  static char s[50];
  strftime(s, sizeof(s), "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));
  return s;
}


time_t TimeTaskClass::getTime() {
  return (lastTimeCheck);
}

time_t TimeTaskClass::getTimeRounded(int multiple) {
  if (multiple == 0)
    return lastTimeCheck;

  int remainder = abs(lastTimeCheck) % multiple;
  if (remainder == 0)
    return lastTimeCheck;

  if (lastTimeCheck < 0)
    return -(abs(lastTimeCheck) - remainder);
  else
    return lastTimeCheck + multiple - remainder;

}


/*
  void TimeTaskClass::printTime() {

  second = timeinfo.tm_sec;
  minute = timeinfo.tm_min;
  hour = timeinfo.tm_hour;
  day = timeinfo.tm_mday;
  month = timeinfo.tm_mon + 1;
  year = timeinfo.tm_year + 1900;
  weekday = timeinfo.tm_wday + 1;
  Serial.print("Time from variables:  ");
  Serial.print(day);
  Serial.print(".");
  Serial.print(month);
  Serial.print(".");
  Serial.print(year);
  Serial.print(" --- ");
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minute);
  Serial.print(":");
  Serial.println(second);
  }
*/
