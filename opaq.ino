
#include"OPAQ_controller.h"

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
  opaq_controller.setup_controller();
  
}

void loop() {
  opaq_controller.run_controller();
}
