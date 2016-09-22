
#include <protocol.h>

#include <Arduino.h>
#include <RtcDateTime.h>
#include <SPI.h>

#include "src/RF24/nRF24L01.h"
#include "src/RF24/RF24.h"
#include "src/ADS7846/ADS7846.h"
#include "gfx.h"

class Opaq_com;

class Opaq_com_nrf24
{
private:
  // Set up nRF24L01 radio on SPI bus plus pins CE=16 & CS=15
  RF24 radio;
public:
  Opaq_com_nrf24() : radio ( RF24 ( 0, 0 ) ) {}
  void init();
  RF24& getRF24() { return radio; };
  void printstate() { radio.printDetails(); };
};

class Opaq_com_atsha204
{
  Opaq_com& com;
public:
  Opaq_com_atsha204(Opaq_com& c) : com(c) {};
  void getCiferKey();
};


struct touch_t {
  unsigned int x;
  unsigned int y;
  unsigned int pressure;
};

class Opaq_com_tsc2046
{
private:
  Opaq_com& com;
  ADS7846 touch;
public:
  Opaq_com_tsc2046(Opaq_com& c) : com(c) {};
  void begin();
  void service();
  void doCalibration(LCD_HAL_Interface& lcd);
  void setCalibration(CAL_MATRIX matrix);
  bool getCalibrationMatrix(CAL_MATRIX& matrix);
  touch_t get();
};

class Opaq_com
{
  bool spi_lock;
  
  public:
  
  Opaq_com();

  bool lock();
  void spinlock();
  void unlock();

  bool connect();
  void disconnect();
  
  void setClock(RtcDateTime c);
  void getClock(RtcDateTime& clock);

  void getCiferKey();

  Opaq_com_nrf24 nrf24;
  Opaq_com_atsha204 atsha204;
  Opaq_com_tsc2046 touch;

};

extern Opaq_com communicate;
