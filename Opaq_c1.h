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

#include "Opaq_com.h"

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <RtcDateTime.h>

#include <Wire.h>
#include <Ticker.h>

#include <atomic>

// ESP8266 specific headers
extern "C" {
#include "user_interface.h"
#include "os_type.h"
}

// permanent storage settings signature (if value is changed then permanent settings will be overwritten by factory default settings)
#define SIG 0x07
#define OPAQ_VERSION "1.0.7"

// configuration parameters
#define OPAQ_MDNS_RESPONDER 1
#define OPAQ_OTA_ARDUINO 0


#define deviceTaskPrio           2
#define deviceTaskQueueLen       1
#define _10hzLoopTaskPrio        1
#define _10hzLoopTaskQueueLen    1


class OpenAq_Controller
{
private:

  AsyncWebServer server;
  AsyncWebSocket ws;

  // Set up alarms to call periodic tasks
  Ticker timming_events;
  Ticker t_evt;

  // real-time clock initialization
  bool clockIsReady;

  // manages flash memory block for permanent settings
  //Opaq_storage storage;

  // temporary string container
  String str;

  // stores current real time clock
  RtcDateTime clock;

  // stores event that triggers deviceTask
  os_event_t deviceTaskQueue[deviceTaskQueueLen];
  os_event_t _10hzLoopQueue[deviceTaskQueueLen];

  void ota();

  void run_programmer();

public:

  OpenAq_Controller();

  void setup_controller();

  void setClockReady() { clockIsReady = true; };
  bool isClockReady() { return clockIsReady; };
  RtcDateTime getClock() { return clock; };

  void run_controller();
  void run_task_ds3231();
  void run_touch();
  void run_tft();

  Opaq_com& getCom() { return communicate; };

  void syncClock();

  AsyncWebServer& getServer() { return server; }
  AsyncWebSocket& getWs() { return ws; }

  void reconnect();

};

extern OpenAq_Controller opaq_controller;

#endif // OPENAQ_H
