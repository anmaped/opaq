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

#ifndef OPENAQ_H
#define OPENAQ_H

#include "AcHtml.h"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <JsonParser.h>
#include <RtcDS3231.h>

#include <nRF24L01.h>
#include <RF24.h>

#include <Wire.h>
#include <Ticker.h>

// ESP8266 specific headers
extern "C" {
#include "user_interface.h"
#include "os_type.h"
}

// permanent storage settings signature (if value is changed then permanent settings will be overwritten by factory default settings)
#define SIG 0x56

#define deviceTaskPrio        1
#define deviceTaskQueueLen    1

class OpenAq_Controller
{
private:

  //JsonParser<32> parser;

  ESP8266WebServer server;

  // Set up alarms to call periodic tasks
  Ticker timming_events;

  // real-time clock initialization
  RtcDS3231 rtc;

  // Set up nRF24L01 radio on SPI bus plus pins CE=16 & CS=15
  RF24 radio;

  // manages html producer
  AcHtml html;

  // manages flash memory block for permanent settings
  AcStorage storage;

  // temporary string container
  String str;

  // stores current real time clock
  RtcDateTime clock;

  // stores event that triggers deviceTask
  os_event_t deviceTaskQueue[deviceTaskQueueLen];

  void handleRoot();
  void handleLight();
  void handleAdvset();
  void handleClock();
  void handlePower();
  void handleGlobal();

  void updatePowerOutlets ( uint8_t pdeviceId );

  void sendBlock ( String* str );

public:

  OpenAq_Controller();

  void factory_defaults ( uint8_t sig );
  void setup_controller();

  void run_controller();
  void run_task_rf433ook();
  void run_task_ds3231();
  void run_task_nrf24();

};

extern OpenAq_Controller opaq_controller;;

#endif // OPENAQ_H
