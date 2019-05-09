#include "Blob.h"
#include "OLED.h"
#include "LoraPublisher.h"
#include <Arduino.h>

LoraPublisher::LoraPublisher(QueueHandle_t readingsQueue)
  : Publisher(readingsQueue)
{}


void LoraPublisher::begin() {
  // check lora is running  
}


void LoraPublisher::publish() {

  BaseType_t xStatus = pdPASS;
  Reading* r;
  int nReadings;
  const TickType_t xTicksToWait = pdMS_TO_TICKS(10);

  nReadings = uxQueueMessagesWaiting(this->_readingsQueue);
  if (nReadings > 0) {
    Log.verbose("Publishing the queue of %d readings\n", nReadings);
  }
  OLED.disableButtons();  // Lora stuffs the capacitance touch

  while (xStatus == pdPASS) {
    xStatus = xQueueReceive(this->_readingsQueue, &r, xTicksToWait);
    if (xStatus == pdPASS) {
      assert(LoRa.beginPacket() == 1);
      LoRa.print(r->toJSON());
      LoRa.endPacket(false);       // blocks until sen
      //Serial.println(r->toJSON());
      delete r; r = NULL;
    }
  }
  OLED.enableButtons();
}
