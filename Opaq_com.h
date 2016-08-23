
#include <protocol.h>

#include <Arduino.h>
#include <RtcDateTime.h>
#include <SPI.h>

#include "src/RF24/nRF24L01.h"
#include "src/RF24/RF24.h"


class Opaq_com_nrf24
{
private:
  // Set up nRF24L01 radio on SPI bus plus pins CE=16 & CS=15
  RF24 radio;
public:
  Opaq_com_nrf24() : radio ( RF24 ( 0, 0 ) ) {}
  void init();
  RF24& getRF24() { return radio; };
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

};

extern Opaq_com communicate;
