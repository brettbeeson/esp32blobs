#include "MQTTPublisher.h"
//#include "OLED.h"
#include "Reading.h"
#include <Arduino.h>

MQTTPublisher::MQTTPublisher(QueueHandle_t readingsQueue, WiFiClient wifiClient,String mqttServer, int mqttPort)
  : Publisher(readingsQueue),
    _mqttClient(wifiClient),
    _mqttServer(mqttServer),
    _mqttPort(mqttPort)
{
  _mqttClient.setServer(_mqttServer.c_str(), _mqttPort);
}

void MQTTPublisher::begin() {
}

void MQTTPublisher::publish() {
  BaseType_t xStatus = pdPASS;
  Reading* r;
  const TickType_t xTicksToWait = pdMS_TO_TICKS(10);

  Log.verbose("Publishing the queue of %d", uxQueueMessagesWaiting(_readingsQueue));
  //OLED.message(String("Publishing ") + uxQueueMessagesWaiting(_readingsQueue));

  while (xStatus == pdPASS) {
    xStatus = xQueueReceive(_readingsQueue, &r, xTicksToWait);
    if (xStatus == pdPASS) {
      // Publish. If unable to publish, save to SD card with serialize for later publication

      reconnect();
      if (!_mqttClient.publish(r->mqttTopic().c_str(), r->mqttValue().c_str())) {
        // try to put back in queue?
        String msg = String("MQTT couldn't publish. Error = ") + String(_mqttClient.state());
        throw std::runtime_error(msg.c_str());
      }
      delete r;	r = NULL;
    }
  }
}

/*
   Todo fail after 3 tries?
*/
void MQTTPublisher::reconnect() {
  // Loop until we're reconnected
  while (!_mqttClient.connected()) {
    Serial.print("Attempting MQTT connection to " );
    Serial.print(_mqttServer); Serial.print(":"); Serial.print(_mqttPort); Serial.print("...");
    // Create a random client ID
    // \todo Name the blob
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (_mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      //client.subscribe("#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(_mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait before retrying
      delay(5000);
    }
  }
}
