/*
 *  Opaq is an Open AQuarium Controller firmware. It has been developed for
 *  supporting several aquarium devices such as ligh dimmers, power management
 *  outlets, water sensors, and peristaltic pumps. The main purpose is to
 *  control fresh and salt water aquariums.
 *
 *    Copyright (c) 2015 Andre Pedro. All rights reserved.
 *
 *  This file is part of opaq firmware for aquarium controllers.
 *
 *  opaq firmware is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  opaq firmware is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with opaq firmware.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
 
#include "Opaq_c1.h"

#include <ESP8266WiFi.h> 
#include <ESP8266WebServer.h>
#include <ESP8266AVRISP.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266httpClient.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <Time.h>

#include <Adafruit_ILI9341.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <ADS7846.h>

#if OPAQ_MDNS_RESPONDER
#include <ESP8266mDNS.h>
#endif

#if OPAQ_OTA_ARDUINO
//#include <ArduinoOTA.h>
#endif

//#include <RtcDS3231.h>
#include <RtcDS1307.h>

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
