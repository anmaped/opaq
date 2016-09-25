
#include "protocol.h"
#include "ds1307.h"
#include "atsha204.h"
#include "rf433.h"


#include <RtcDS1307.h>
#include <Wire.h>
#include <SPI.h>
#include <Scheduler.h>

#include <SHA204.h>
#include <SHA204Definitions.h>
#include <SHA204I2C.h>

// pinout
#define PIN_NRF24_CSN 7
#define PIN_NRF24_CE 8
#define PIN_TOUCH_CS A1

#define RED_LED 5
#define GREEN_LED 6
#define BLUE_LED 9

volatile byte command = 0;
volatile bool comunicating = 0;
volatile byte count = 0;

// the setup function runs once when you press reset or power the board
void setup() {

  delay(100);

  Serial.begin(9600);
  
  pinMode(11, INPUT);
  pinMode(12, OUTPUT);
  pinMode(13, INPUT);
  pinMode(10, INPUT);
  
  // turn on SPI in slave mode
  SPCR |= _BV(SPE);

  // turn on interrupts
  SPCR |= _BV(SPIE);

  /**
   * Pin Change Interrupt enable on PCINT0 (PB0)
   */
  PCICR |= _BV(PCIE0);
  PCMSK0 = 0;
  PCMSK0 |= _BV(PCINT2);

  // Turn interrupts on.
  sei();
  
  // initialize digital pin 13 as an output.
  pinMode(5, OUTPUT); // green
  pinMode(6, OUTPUT); // red
  pinMode(9, OUTPUT); // blue

  pinMode(7, OUTPUT); // pin NRF24_CSN
  pinMode(8, OUTPUT); // pin NRF24_CE
  pinMode(PIN_TOUCH_CS, OUTPUT); // pin TOUCH_CS
  digitalWrite(7, HIGH);
  digitalWrite(PIN_TOUCH_CS, HIGH);

  pinMode(3, OUTPUT);
  analogWrite(3, 255);

  // real-time clock
  rtc.Begin();
  //rtc.SetDateTime(RtcDateTime(2016,5,25,5,34,0));
  rtc.SetIsRunning(true);
  
  sha204dev.init();
  wakeupExample(); // dummy
  serialNumberExample(); // dummy

  
  // RF433 setup
  Rf433_transmitter.set_pin ( A0 );
  Rf433_transmitter.set_encoding ( Rf433_transmitter.CHANON_DIO_DEVICE );


  
  Scheduler.start(setup_status, loop_status);
  Scheduler.start(setup_com, loop_com);
  Scheduler.start(setup_warning, loop_warning);
}

void setup_status()
{
  pinMode(GREEN_LED, OUTPUT);
}

void loop_status() {
  if (!comunicating)
  {
    blink(GREEN_LED, 500);
  }
  else
  {
    digitalWrite(GREEN_LED, LOW);
    delay(1000);
  }
}

void setup_com()
{
  pinMode(BLUE_LED, OUTPUT);
}

void loop_com() {
  digitalWrite(BLUE_LED, LOW);
  delay(100);
  digitalWrite(BLUE_LED, HIGH);
  delay(500);
  digitalWrite(BLUE_LED, LOW);
  delay(100);
  digitalWrite(BLUE_LED, HIGH);
  delay(2000);
}


void setup_warning()
{
  pinMode(RED_LED, OUTPUT);
}

void loop_warning() {
  if(true)
  {
    digitalWrite(RED_LED, HIGH);
    delay(1000);
  }
  else
  {
    blink(RED_LED, 500);
  }
}

void blink(int pin, unsigned int ms)
{
  digitalWrite(pin, HIGH);
  delay(ms);
  digitalWrite(pin, LOW);
  delay(ms);
}


ISR(PCINT0_vect) {
  if(!(PINB & B00000100))
  {
    PORTD |= B10000000;
    PORTC |= B00000010;
  }
  else
  {
    command = 0;
  }
}

// SPI interrupt routine
ISR (SPI_STC_vect)
{ 
  byte data = SPDR;
  
  switch (command)
  {
  // no command? then this is the command
  case 0:
    SPDR = 1;
    command = data;
    count = 0;
    break;
    
  case ID_DS1307_GETDATA:
    
    if(count < sizeof(RtcDateTime))
      SPDR = ds1307(count);

    count++;
    comunicating = true;
    break;

  case ID_DS1307_SETDATA:
  
    if(count < sizeof(RtcDateTime))
    {
      setDs1307(data, count);
      
      if(count == sizeof(RtcDateTime)-1)
      {
        // update clock
         setClockActive = true;
      }
    }
    
    count++;
    break;

  case ID_NRF24_CSN:

    if(count == 0)
    {
      if(data == 0x01)
      {
        //digitalWrite(PIN_NRF24_CSN, HIGH);
        PORTD |= B10000000;
      }
      else
      {
        //digitalWrite(PIN_NRF24_CSN, LOW);
        PORTD &= B01111111;
      }
    }

    command=0;
    break;

  case ID_NRF24_CE:

    if(data == HIGH)
    {
      digitalWrite(PIN_NRF24_CE, HIGH);
    }
    else
    {
      digitalWrite(PIN_NRF24_CE, LOW);
    }

    command=0;
    break;

  case ID_ATSHA402:

    SPDR = serialNumber[count];

    count++;
    break;

  case ID_TSC2046_CSN:
  
     if(data == HIGH)
    {
      //digitalWrite(PIN_TOUCH_CS, HIGH);
      PORTC |= B00000010;
    }
    else
    {
      //digitalWrite(PIN_TOUCH_CS, LOW);
      PORTC &= B11111101;
    }
    
    command=0;
    break;

  case ID_RF433_STREAM:

    //byte data = SPDR;
    // enqueue stream
    enqueue_stream(data, count);
    SPDR = data;

    count++;
    break;

  case ID_DIM_TFT:

    analogWrite(3, data);
    break;

  } // end of switch

}  // end of interrupt service routine (ISR) SPI_STC_vect


void loop() {
  
  delay(10);

  if(setClockActive)
  {
    setClockActive = false;
    rtc.SetDateTime(clock);
  }
  else
  {
    clock = rtc.GetDateTime();
  }

  rf433();
  
}

