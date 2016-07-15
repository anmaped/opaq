
#include "Opaq_storage.h"

#include <protocol.h>

#include <Arduino.h>
#include <RtcDateTime.h>
#include <SPI.h>

class Opaq_com
{
  bool spi_lock;
  
  bool connect();
  void disconnect();
  
  public:
  
  Opaq_com();

  bool lock();
  void spinlock();
  void unlock();
  
  void setClock(RtcDateTime c);
  void getClock(RtcDateTime& clock);

  void getCiferKey();
  void rf433(uint32_t code, uint8_t state);

};

