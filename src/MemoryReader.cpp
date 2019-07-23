#include "MemoryReader.h"


// Allows task to wait on queue forever
#define INCLUDE_vTaskSuspend                    1

// measurements[3
MemoryReader::MemoryReader(Blob* blob)
  : Reader(blob)
{
}

void MemoryReader::begin() {

}

int MemoryReader::read() {
  BaseType_t xStatus=0;
  sample_t* samples=NULL;
  Reading *r=NULL;
  int s, m;
  int nSent;
  int nRead = 0;

  for ( m = 0; m < N_MEASUREMENTS; m++) {
    nSent = 0;

    samples = rtcmem::measurements[m].samples;
    assert(samples);
    //debugV("MemoryReader::read(): rtcmem::measurements[%d]->samples:%d\n", m, samples);
    // todo set #samples in measurement?
    if (rtcmem::measurements[m].id[0] != 0)     {
      for ( s = 0; s < N_SAMPLES; s++) {
        if (samples[s].t != 0) {
          r = new Reading(&rtcmem::measurements[m], samples[s]);
          nRead++;
          // dequeuer will delete
          //
          // don't use block - return to caller instead
          if (xQueueSendToBack(_readingsQueue, &r, pdMS_TO_TICKS(100) ) == pdPASS) {
            // ok
            samples[s].t = 0;
            samples[s].v = 0;
            nSent++;
          } else {
            delete(r); r = NULL;
            debugI("MemoryReader not send to the queue. Can retry. Full = %d", xStatus == errQUEUE_FULL);
            return -1;
          }
        }
      }
      //debugV("Sent: %d Failed: %d samples for measurement %d\n", nSent, nFailed, m);
    } // (rtcmem::measurements[m].id[0] != 0)

  }
  return nSent;
}

int MemoryReader:: nTotalSamples() {
  
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
