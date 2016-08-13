
#include <protocol.h>

#include <Arduino.h>
#include <RtcDateTime.h>
#include <SPI.h>


class Opaq_com_rf433
{
  public:
  Opaq_com_rf433() {};
  void send(unsigned int code, short unsigned int state);
  bool ready();
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
  //void rf433(uint32_t code, uint8_t state);

  Opaq_com_rf433 rf433;

};

extern Opaq_com communicate;
