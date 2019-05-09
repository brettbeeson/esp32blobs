#pragma once

#include "Blob.h"
#include "Publisher.h"
#include "ESPinfluxdb.h"

class InfluxPublisher : public Publisher {
  
  public:
    InfluxPublisher(QueueHandle_t readingsQueue,String _influxServer,int port, String _db, String _measurement,String _user,String _password);   
    void begin();
    void publish();
    
  private:
    Influxdb influxdb;
    String db;
    String user;
    String password;
    String measurement;
};
