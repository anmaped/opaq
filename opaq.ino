
#include"OpenAq_controller.h"

#include <ESP8266WiFi.h> 
#include <ESP8266WebServer.h>
#include <JsonParser.h>

#include <RtcDS3231.h>

#include <nRF24L01.h>
#include <RF24.h>

#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Ticker.h>

#define DEBUG

void setup() {
  Controller.setup_controller();
  
}

void loop() {
  Controller.run_controller();
}
