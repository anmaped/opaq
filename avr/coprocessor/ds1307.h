
#include <arduino.h>
#include <RtcDS1307.h>

RtcDS1307 rtc;
RtcDateTime clock;

byte ds1307(byte code)
{
  byte * clock_pointer = (byte *)&clock;
  return clock_pointer[code];
}

