#include <DallasTemperature.h>
#include "DSTempSensors.h"
//#include "OLED.h"
#define MAX_POSSIBLE_SENSORS 10

// temp only
String addrToString(const DeviceAddress a) {
  char s[17];
  for (int i = 0; i < 8; i++)  {
    snprintf(&(s[i * 2]), 3 /* 2 \0 */, "%02X", a[i]);
  }
  return String(s);
}

/**
   DSTempSensor
*/
DSTempSensor :: DSTempSensor (DallasTemperature *sensorBus, DeviceAddress addr) :
  _calibrated(false),
  _calibration(0.0),
  _sensorBus(sensorBus) {

  // copy addr : it's an array
  memcpy(_addr, addr, AddrLength);
  // make _addrCStr
  for (int i = 0; i < AddrLength; i++)  {
    snprintf(&(_addrCStr[i * 2]), 3 /* 2 \0 */, "%02X", _addr[i]);
  }
  // look up calibration
  Log.warning("*\n*\n Calibration not working!\n*\n*");
  bool match = true;
  for (int i = 0; i < sizeof(CalibrationTable) / sizeof(CaliT); i++) {
    match = true;
    for (int j = 0; j < 8; j++) { // each byte
      match = match && (CalibrationTable[i].addr[j] == _addr[j]);
    }
    if (match) {
      _calibration = CalibrationTable[i].offset;
      _calibration = true;
    }
  }
}

float DSTempSensor::getTempC() {
  return (_sensorBus->getTempC(_addr) + _calibration);
}

const char* DSTempSensor:: getAddrCStr() {
  return _addrCStr;
}

/**
   DSTempSensors
*/

DSTempSensors::DSTempSensors(QueueHandle_t readingsQueue, int pin) :
  Sensor (readingsQueue), _pin(pin) {
}

int DSTempSensors::nReadings()   {
  return _sensors.size();
}

/*
*/
void DSTempSensors::begin() {

  DeviceAddress addr;

  Log.verbose("DSTempSensors::begin() Executing on core %d off pin %d\n", xPortGetCoreID(), _pin);

  _oneWire = new OneWire(_pin);
  _sensorBus = new DallasTemperature(_oneWire);
  _sensorBus->setResolution(9); // 9,10,11,12
  _sensorBus->begin();
  _sensorBus->requestTemperatures();
  // Not reliable
  //nSensors = sensors->getDS18Count();
  //nSensors = sensors->getDeviceCount();
  // May not be reliable
  bool p = _sensorBus->isParasitePowerMode();

  // Reliable - first run through all indexes, then get the address of each
  int n;
  for (n = 0; n <= MAX_POSSIBLE_SENSORS && _sensorBus->getTempCByIndex(n) != DEVICE_DISCONNECTED_C; n++) {};

  Log.verbose("DSTempSensors::begin(): found %d sensors, parasitic power *= %d\n", n, p);

  for (int i = 0; i < n; i++) {
    if (_sensorBus->getAddress(addr, i)) {
      _sensors.emplace_back(_sensorBus, addr);
      Log.verbose("Found DS[ % d] : % s\n", i, _sensors.back().getAddrCStr() );
      if (!_sensors.back()._calibrated) {
        Log.warning("No calibration for DS:%s\n", _sensors.back().getAddrCStr());
      }
      //OLED.message("DST: " + String(_sensors.back().getAddrCStr()).substring(12, 16) + ": " + _sensors.back()._calibration);
    } else {
      Log.warning("Phantom address DS #%d\n", i); //, addrToString(addr).c_str());
    }
  }

  Sensor::begin(); // malloc readings and default; need to call once nReadings() is known, which is size of vector

  for (int i = 0; i < _sensors.size(); i++) {
    readings[i]->metric = String("temperature");
    readings[i]->units = String("C");
    readings[i]->id = String(_sensors[i].getAddrCStr());
  }

  for (int i = 0; i < _sensors.size(); i++) {
    Log.verbose("Sensor % i: Ptr = % d Addr = % s Cali = % f\n", i, &_sensors[i], _sensors[i].getAddrCStr(), _sensors[i]._calibration);
    Log.verbose("Reading % i: Ptr = % d ToStr = % s\n", i, &(readings[i]), readings[i]->tostr().c_str());
  }
  //  OLED.message("DSTempSensor: found " + String(n));
}
/*
  \todo Consider ASYNC to save energy
*/
void DSTempSensors::read() {

  double temp = 666;

  _sensorBus->requestTemperatures();
  for (int i = 0; i < nReadings(); i++) {
    temp = _sensors[i].getTempC();
    if (temp == DEVICE_DISCONNECTED_C || temp < -100 || temp > 100)   {
      Log.warning("Could not read temperature data ( % f) on unit #%d : %s\n", temp, i, _sensors[i].getAddrCStr() );
      readings[i]->value = NAN;
    } else {
      readings[i]->value = temp;
    }
  }
}
