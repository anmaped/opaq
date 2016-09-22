

#include "Opaq_com.h"
#include "cas.h"

#include <Arduino.h>

Opaq_com communicate;
  

Opaq_com::Opaq_com() : nrf24 (Opaq_com_nrf24()), atsha204(Opaq_com_atsha204(communicate)), touch(Opaq_com_tsc2046(communicate))
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
  while(!cas((uint8_t*)&spi_lock, false, true)) { yield(); }
}

void Opaq_com::unlock()
{
  if (spi_lock)
    spi_lock = false;
}

bool Opaq_com::connect()
{
  spinlock();

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

void Opaq_com_nrf24::init()
{
  // NRF24 setup and radio configuration
  radio.begin();
  radio.setChannel(1);
  radio.setDataRate(RF24_1MBPS);
  radio.setAutoAck(false);
  //radio.disableCRC();
  radio.openWritingPipe( 0x5544332211LL ); // set address for outcoming messages
  radio.openReadingPipe( 1, 0x5544332211LL ); // set address for incoming messages

  // manual test
  //uint8_t buf[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
  //radio.write_register ( TX_ADDR, buf, 5 );
  //radio.write_register ( SETUP_AW, 0x3 );
  //radio.write_register ( EN_AA, 0x0 ); // mandatory - no ACK
  //radio.write_register ( EN_RXADDR, 0x0 );
  //radio.write_register ( SETUP_RETR, 0x0 );
  //radio.write_register ( RF_CH, 0x1 );
  //radio.write_register ( RF_SETUP, 0x7 );
  //radio.write_register ( CONFIG, 0xE ); // mandatory

  //radio.startListening();

  // for debug purposes of radio transceiver
  //radio.printDetails();
}

void Opaq_com_atsha204::getCiferKey()
{
  byte x[3];
  
  //Serial.println(F("ASK AVR FOR ATSHA204"));

  if( com.connect() )
    return;
  
  SPI.transfer (ID_ATSHA402);
  delayMicroseconds (30);
  
  for(byte i=0; i <= 9; i++)
  {
    x[i] = SPI.transfer (0);
    delayMicroseconds (30);
  }

  com.disconnect();

  /*for(byte i=0; i <= 9; i++)
    Serial.println("r: " + String(x[i]));
*/
  //Serial.println(F("ATSHA402 ASKED!"));
}



void Opaq_com_tsc2046::service()
{
  if(com.lock())
    return;
  
  touch.service();
/*
  Serial.println("ASK TOUCH:");
  Serial.println(touch.tp_x);
  Serial.println(touch.tp_y);
  Serial.println(touch.pressure);
  Serial.println(touch.getX());
  Serial.println(touch.getY());
  Serial.println("TOUCH ASKED!");
  */

  com.unlock();
}

void Opaq_com_tsc2046::begin()
{
  com.spinlock();
  touch.begin();
  com.unlock();
}

void Opaq_com_tsc2046::doCalibration(LCD_HAL_Interface& lcd)
{
  com.spinlock();
  touch.doCalibration(&lcd);
  com.unlock();
}

void Opaq_com_tsc2046::setCalibration(CAL_MATRIX matrix)
{
  com.spinlock();
  touch.setCalibration(matrix);
  com.unlock();
}

touch_t Opaq_com_tsc2046::get()
{
  touch_t data = {0};
  if(com.lock())
    return data;

  data.x = touch.getX();
  data.y = touch.getY();
  data.pressure = touch.getPressure();
  com.unlock();

  return data;
}

bool Opaq_com_tsc2046::getCalibrationMatrix(CAL_MATRIX& matrix)
{
  com.spinlock();
  bool result = touch.getCalibrationMatrix(matrix);
  com.unlock();

  return result;
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

  //Serial.println(F("ASK AVR FOR DS3231 GET"));

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

  /*for(byte i=0; i <= sizeof(RtcDateTime); i++)
  {
    Serial.println("r: " + String(x[i]));
  }*/

  //Serial.println(F("AVR ASKED!"));
  
  clock = RtcDateTime(x[1], x[2], x[3], x[4], x[5], x[6]);
  
}
