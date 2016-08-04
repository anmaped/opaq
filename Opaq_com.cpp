

#include "Opaq_com.h"
#include "cas.h"

#include <Arduino.h>


Opaq_com::Opaq_com()
{
  spi_lock = false;
}

bool Opaq_com::lock()
{
  if(!cas((uint8_t*)&spi_lock, false, true))
    return true;

  return false;
}

void Opaq_com::spinlock()
{
  while(!cas((uint8_t*)&spi_lock, false, true)) { delay(100); }
}

void Opaq_com::unlock()
{
  if (spi_lock)
    spi_lock = false;
}

bool Opaq_com::connect()
{

  if( lock() )
    return true;

  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

  digitalWrite(5, LOW);
  delayMicroseconds (20);

  return false;
  
}

void Opaq_com::disconnect()
{
  digitalWrite(5, HIGH);

  SPI.endTransaction();
  
  unlock();
}


void Opaq_com::setClock(RtcDateTime c)
{
  Serial.println(F("ASK AVR FOR DS3231 SET"));

  if( connect() )
    return;
  
  SPI.transfer (ID_DS1307_SETDATA);
  delayMicroseconds (30);

  byte *x = (byte*)&c; // this is not safe ... alignment dependent
  
  for(byte i=0; i < sizeof(RtcDateTime); i++)
  {
    SPI.transfer (x[i]);
    delayMicroseconds (30);
  }

  disconnect();
}

void Opaq_com::getClock(RtcDateTime& clock)
{
  //bool ee = true;
  //bool &e = ee;
  //spi_lock.compare_exchange_strong(e, false);

  byte x[sizeof(RtcDateTime) + 1];

  Serial.println(F("ASK AVR FOR DS3231 GET"));

  if( connect() )
    return;

  
  SPI.transfer (ID_DS1307_GETDATA);
  delayMicroseconds (30);
  
  for(byte i=0; i <= sizeof(RtcDateTime); i++)
  {
    x[i] = SPI.transfer (0);
    delayMicroseconds (30);
  }

  disconnect();

  for(byte i=0; i <= sizeof(RtcDateTime); i++)
  {
    Serial.println("r: " + String(x[i]));
  }

  Serial.println(F("AVR ASKED!"));
  
  clock = RtcDateTime(x[1], x[2], x[3], x[4], x[5], x[6]);
  
}


void Opaq_com::getCiferKey()
{
  byte x[3];
  
  Serial.println(F("ASK AVR FOR ATSHA204"));

  if( connect() )
    return;
  
  SPI.transfer (ID_ATSHA402);
  delayMicroseconds (30);
  
  for(byte i=0; i <= 9; i++)
  {
    x[i] = SPI.transfer (0);
    delayMicroseconds (30);
  }

  disconnect();

  for(byte i=0; i <= 9; i++)
    Serial.println("r: " + String(x[i]));

  Serial.println(F("ATSHA402 ASKED!"));
}

void Opaq_com::rf433(uint32_t code, uint8_t state)
{
  byte tmp[10];
  byte idx = 0;
  
  // comunicate with coprocessor
  Serial.println(F("ASK AVR TO RECEIVE RF433 STREAM"));

  if( connect() )
    return;

  tmp[idx++] = SPI.transfer (ID_RF433_STREAM);
  delayMicroseconds (30);

  // payload length
  tmp[idx++] = SPI.transfer ( 0x05 );
  delayMicroseconds (30);
  
  tmp[idx++] = SPI.transfer ( (byte)code );
  delayMicroseconds (30);
  tmp[idx++] = SPI.transfer ( (byte)(code >> 8) );
  delayMicroseconds (30);
  tmp[idx++] = SPI.transfer ( (byte)(code >> 16) );
  delayMicroseconds (30);
  tmp[idx++] = SPI.transfer ( (byte)(code >> 24) );
  delayMicroseconds (30);
  tmp[idx++] = SPI.transfer ( state );
  delayMicroseconds (30);

  // dummy
  tmp[idx++] = SPI.transfer ( 0x10 );
  delayMicroseconds (30);
  
  disconnect();

  for(byte i=0; i < idx; i++)
    Serial.println("r: " + String(tmp[i]));

  Serial.println(F("AVR ASKED!"));
}

