
#include <arduino.h>
#include <RtcDS1307.h>

RtcDS1307 rtc;
RtcDateTime clock;

volatile bool setClockActive = false;
extern volatile bool setClockActive;

byte ds1307(byte code)
{
  setClockActive = false;
  
  byte * clock_pointer = (byte *)&clock;
  
  return clock_pointer[code];
}

void setDs1307(byte code, byte idx)
{
  setClockActive = true;
  
  byte * clock_pointer = (byte *)&clock;

  clock_pointer[idx] = code;
  
}

