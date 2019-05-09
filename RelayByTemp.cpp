/*

    Created on: 8 Jan 2019
        Author: bbeeson
*/

#include "Blob.h"
#include "RelayByTemp.h"

//

RelayByTemp ::RelayByTemp (int pin,DSTempSensors *tempSensor):
  _pin(pin),
  _tempSensor(tempSensor) {
}


void RelayByTemp ::begin() {
}

void RelayByTemp ::adjust() {
 
}

RelayByTemp ::~RelayByTemp () {

}
