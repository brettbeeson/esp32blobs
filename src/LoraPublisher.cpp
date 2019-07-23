#include "Blob.h"
#include "OLED.h"
#include "LoraPublisher.h"
#include <Arduino.h>

LoraPublisher::LoraPublisher(Blob* blob)
  : Publisher(blob)
{}


void LoraPublisher::begin() {
  // check lora is running  
}


int LoraPublisher::publish() {

  BaseType_t xStatus = pdPASS;
  Reading* r;
  int nReadings;
  const TickType_t xTicksToWait = pdMS_TO_TICKS(10);
  int nPublished=0;

  nReadings = uxQueueMessagesWaiting(this->_readingsQueue);
  if (nReadings > 0) {
    debugV("Publishing the queue of %d readings\n", nReadings);
  }
  OLED.disableButtons();  // Lora stuffs the capacitance touch

  while (xStatus == pdPASS) {
    xStatus = xQueueReceive(this->_readingsQueue, &r, xTicksToWait);
    if (xStatus == pdPASS) {
      assert(LoRa.beginPacket() == 1);
      LoRa.print(r->toJSON());
      LoRa.endPacket(false);       // blocks until sen
      nPublished++;
      //Serial.println(r->toJSON());
      delete r; r = NULL;
    }
  }
  OLED.enableButtons();
  return nPublished;
}
