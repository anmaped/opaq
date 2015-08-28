
#include"Rf433ook.h"

#include<stdbool.h>
#include <Arduino.h>

Rf433ook::Rf433ook()
{
  
}

void Rf433ook::set_pin(int pinout)
{
  // configure pin
  pinMode(pinout,OUTPUT);
  
  pin = pinout;
}

void Rf433ook::set_encoding(int device)
{
  encoding=device;
}

void Rf433ook::sendOOK(bool b)
{
  // EasyHome case
  if (encoding == CHANON_DIO_DEVICE)
  {
    if (b) {
        digitalWrite(pin, HIGH);
        delayMicroseconds(275);   //275 originally, but tweaked.
        digitalWrite(pin, LOW);
        delayMicroseconds(1225);  //1225 originally, but tweaked.
      } else {
        digitalWrite(pin, HIGH);
        delayMicroseconds(275);   //275 originally, but tweaked.
        digitalWrite(pin, LOW);
        delayMicroseconds(275);   //275 originally, but tweaked.
      }
    }
    
    // other cases...
}

void Rf433ook::sendBit(bool data_bit) {
  
  // EasyHome case
  if (encoding == CHANON_DIO_DEVICE)
  {
    // case encoding is manchester
    // Manchester bit 0 is High for 275uS and Low for 275uS
    // Manchester bit 1 is High for 275uS and Low for 1225uS
    // data bit 0 = Manchester encoding '01'
    // data bit 1 = Manchester encoding '10'
    // data bit 0 = High 275uS, Low 275uS, High 275uS, Low 1225uS
    // data bit 1 = High 275uS, Low 1225uS, High 275uS, Low 275uS
    
    if (data_bit) {
      sendOOK(true);
      sendOOK(false);
    } else {
      sendOOK(false);
      sendOOK(true);
    }
  }

  // other cases...
  
}

void Rf433ook::sendMessage(bool bit_vector[]) {
  
  // EasyHome case
  if (encoding == CHANON_DIO_DEVICE)
  {
    // preamble is High 275uS, Low 2675uS
    digitalWrite(pin, HIGH);
    delayMicroseconds(275);
    digitalWrite(pin, LOW);
    delayMicroseconds(2675);
    
    // transmitter ID
    for (int i = 0; i < 26; i++) {
      sendBit(bit_vector[i]);
    }
    
    // bit 26 is the Group Flag,
    sendBit(bit_vector[26]);
    
    // bit 27 is the ON / OFF Flag
    sendBit(bit_vector[27]);
 
    // bits 28 - 31, are the Group Code (up to 16 units can be in the same group) 
    for (int i = 28; i < 32; i++) {
        sendBit(bit_vector[i]);
    }
    
    // bits 32 - 35 are the dim level (16 Levels)
    // unavailable for power socket outlets
    
    digitalWrite(pin, HIGH);   // lock - end of data
    delayMicroseconds(275);    // wait
    digitalWrite(pin, LOW);    // lock - end of signal
  }
  
  // other messages...
}

Rf433ook Rf433_transmitter;

