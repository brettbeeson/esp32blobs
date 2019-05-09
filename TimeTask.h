#ifndef TIMETASK_H
#define TIMETASK_H

#include <time.h>
#include <Arduino.h>
/*
  struct tm
  {
  int    tm_sec;   //   Seconds [0,60].
  int    tm_min;   //   Minutes [0,59].
  int    tm_hour;  //   Hour [0,23].
  int    tm_mday;  //   Day of month [1,31].
  int    tm_mon;   //   Month of year [0,11].
  int    tm_year;  //   Years since 1900.
  int    tm_wday;  //   Day of week [0,6] (Sunday =0).
  int    tm_yday;  //   Day of year [0,365].
  int    tm_isdst; //   Daylight Savings flag.
  }
*/
class TimeTaskClass {
  public:
    TimeTaskClass();
    ~TimeTaskClass();
    void begin(long  gmtOffset_sec, int   daylightOffset_sec = 0,const char* _ntpServer = "pool.ntp.org");
    void begin(const char* tz,const char* _ntpServer = "pool.ntp.org");
    void refresh();
    void sync();
    const char* getLocalTimeStr(time_t t);
    const char* getUTCTimeStr(time_t t);
    
    const char* getLocalTimeStr();
    const char* getUTCTimeStr();
    time_t getTime(); // Seconds since unix epoch
    time_t getTimeRounded(int roundSeconds); // Seconds since unix epoch
    
  private:
    void waitForSet(int msToWait= 10000);
    time_t lastTimeCheck=0;       // timestamp of last refresh
    long refreshPeriodMs = 10000;
    const char* tz=NULL;
    const char* ntpServer = "pool.ntp.org";
    long  gmtOffset_sec = 3600;
    int   daylightOffset_sec = 0;
};

extern TimeTaskClass TimeTask;
/*
  class TimeTask {
  public:
    static TimeTaskClass *instance;
    static void printTime() {
      singleton().printTime();
    }
    static TimeTaskClass& singleton() {
      if (instance==0) instance = new TimeTaskClass();
      return *instance;
    }
  };
*/
#endif
