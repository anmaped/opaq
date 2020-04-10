
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// protocol file between esp8266 and avr

// query
enum ID {ID_DS1307_GETDATA=0x16, ID_DS1307_SETDATA, ID_NRF24_CSN, ID_NRF24_CE, ID_ATSHA402, ID_TSC2046_CSN, ID_RF433_STREAM, ID_DIM_TFT, ID_STATUS};

enum rf433state {RF433_OFF=0, RF433_ON, RF433_BINDING, RF433_UNBINDING};

struct status {
  uint8_t version[3];
  uint8_t uniqueid[4];
  uint8_t mode;
} ;

#endif // PROTOCOL_H
