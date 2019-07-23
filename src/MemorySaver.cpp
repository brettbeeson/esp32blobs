#include "MemorySaver.h"
#include "TimeKeeper.h"
#include <math.h>
#include <Arduino.h>
#include <string.h>

namespace rtcmem {
RTC_DATA_ATTR measurement_t measurements[N_MEASUREMENTS];
RTC_DATA_ATTR sample_t samples[N_MEASUREMENTS][N_SAMPLES];
}

MemorySaver:: MemorySaver(Blob *blob):
  Publisher(blob)
{
}

void MemorySaver::init() {

  for (int m = 0; m < N_MEASUREMENTS; m++) {
    memset(rtcmem::measurements[m].id, 0, READING_STR);
    memset(rtcmem::measurements[m].units, 0, READING_STR);
    memset(rtcmem::measurements[m].metric, 0, READING_STR);
    memset(rtcmem::measurements[m].location, 0, READING_STR);

    rtcmem::measurements[m].samples = rtcmem::samples[m];
    //debugV("MemorySaver::init(): measurements[%d].samples=%d\n", m, rtcmem::measurements[m].samples);
    for (int s = 0; s < N_SAMPLES; s++) {
      rtcmem::measurements[m].samples[s].v = NAN;
      rtcmem::measurements[m].samples[s].t = 0;
    }
  }
}

void MemorySaver::begin() {
}

void MemorySaver::print() {
  //char timeStr[256];
  //Serial.printf("MemorySaver #m: %d #s:%d\n", N_MEASUREMENTS, N_SAMPLES);
  for (int m = 0; m < N_MEASUREMENTS; m++) {
    debugV("MemorySaver::print(): measurements[%d].samples=%d\n", m, (int) rtcmem::measurements[m].samples);
    assert(rtcmem::measurements[m].samples != NULL);
    if (rtcmem::measurements[m].id[0] != 0)     {
      for (int s = 0; s < N_SAMPLES; s++) {
        if (rtcmem::measurements[m].samples[s].t != 0) {
          //strncpy(timeStr, ctime(&rtcmem::measurements[m].samples[s].t), 255);
          //timeStr[strlen(timeStr) - 1] = '\0';
          Serial.printf("m:%d s:%d : t:%lu (%s) v:%f\n", m, s, rtcmem::measurements[m].samples[s].t, TimeKeeper.dateTime(rtcmem::measurements[m].samples[s].t).c_str(), rtcmem::measurements[m].samples[s].v);
        } else {
          //Serial.printf("No time for sample:%d\n",s);
        }
      }
    } else {
      Serial.printf("No ID for measurement:%d\n", m);
    }
  }
}

measurement_t* MemorySaver::findMeasurement(String id) {
  for (int m = 0; m < N_MEASUREMENTS; m++) {
    //debugV("Finding at measurement %d id: %s, looking for id:%s\n", m, rtcmem::measurements[m].id, id.c_str());
    if (strcmp(id.c_str(), rtcmem::measurements[m].id) == 0) {
      //debugV("Found measurement %d for id:%s at %d\n", m, id.c_str(), &(rtcmem::measurements[m]));
      return &(rtcmem::measurements[m]);
    }
  }
  return NULL;
}

measurement_t* MemorySaver::addMeasurement(String id) {

  //assert(findMeasurement(id) == false);
  for (int m = 0; m < N_MEASUREMENTS; m++) {
    if (rtcmem::measurements[m].id[0] == 0) {
      strncpy(rtcmem::measurements[m].id, id.c_str(), READING_STR - 1);
      //debugV("AddedMeasurement %d for id:%s\n", m, id.c_str());
      return &(rtcmem::measurements[m]);
    }
  }
  return NULL;
}


bool MemorySaver::publishReading(Reading * r) {

  measurement_t *m;
  //debugV("Adding reading %s\n", r->tostr().c_str());
  if (!(m = findMeasurement(r->id))) {
    if (!(m = addMeasurement(r->id))) {
      debugE("No space for more measurements in RTC_DATA\n");
      return false;
    }
  }

  //todo move to readings.h
  //r = m;
  strncpy(m->id, r->id.c_str(), READING_STR - 1);
  strncpy(m->location, r->location.c_str(), READING_STR - 1);
  strncpy(m->metric, r->metric.c_str(), READING_STR - 1);
  strncpy(m->units, r->units.c_str(), READING_STR - 1);

  //sample_t *samples;

  for (int s = 0; s < N_SAMPLES; s++) {
    if (m->samples[s].t == 0) {
      m->samples[s].v = r->getValue();
      m->samples[s].t = r->timestamp;
      return true;
    }
  }
  // could find the oldest and add there...
  debugE("No space for more measurement samples in RTC_DATA");
  return false;
}


int MemorySaver:: nTotalSamples() {

  int nSamples = 0;
  sample_t* samples;

  for ( int m = 0; m < N_MEASUREMENTS; m++) {
    samples = rtcmem::measurements[m].samples;
    assert(samples);
    if (rtcmem::measurements[m].id[0] != 0)     {
      for ( int s = 0; s < N_SAMPLES; s++) {
        if (samples[s].t != 0) {
          nSamples++;
        }
      }
    }
  }
  return nSamples;
}

int MemorySaver:: nMeasurements() {

  int nMeasurements = 0;

  for ( int m = 0; m < N_MEASUREMENTS; m++) {
    if (rtcmem::measurements[m].id[0] != 0)     {
      nMeasurements++;
    }
  }
  return nMeasurements;
}
