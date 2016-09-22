

#include <SHA204.h>
#include <SHA204Definitions.h>
#include <SHA204I2C.h>

SHA204I2C sha204dev;
uint8_t response[SHA204_RSP_SIZE_MIN];
uint8_t serialNumber[9];

byte wakeupExample() {
  //uint8_t response[SHA204_RSP_SIZE_MIN];
  byte returnValue;
  
  returnValue = sha204dev.resync(4, &response[0]);
  for (int i=0; i<SHA204_RSP_SIZE_MIN; i++) {
    Serial.print(response[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  return returnValue;
}

byte serialNumberExample() {
  
  byte returnValue;
  
  returnValue = sha204dev.serialNumber(&serialNumber[0]);
  /*for (int i=0; i<9; i++) {
    Serial.print(serialNumber[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
 
  Serial.println("-------"); 
  */
  return returnValue;
}
