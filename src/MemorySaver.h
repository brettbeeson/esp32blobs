#pragma once

#include "Reading.h"
#include "Publisher.h"

// Todo: how to use #ifndef or similiar to allow override?
// Must be static for RTC_MEM
// 
#define N_MEASUREMENTS 5
#define N_SAMPLES 30


namespace rtcmem {
  RTC_DATA_ATTR extern measurement_t measurements[N_MEASUREMENTS];
}


class MemorySaver: public Publisher {

  public:

    MemorySaver(Blob *blob);
    void begin();
    bool publishReading(Reading *r);
    static void print();
    static void init();
    int nMeasurements();
    int nTotalSamples();

  private:

    measurement_t* findMeasurement(String id);
    measurement_t* addMeasurement(String id);
    
};
