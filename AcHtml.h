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

#ifndef ACHTML_H
#define ACHTML_H

#include "AcStorage.h"

#include <String.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <RtcDS3231.h>

class AcHtml {
public:
  AcHtml();

  const __FlashStringHelper* get_begin();
  const __FlashStringHelper* get_end();
  const __FlashStringHelper* get_header();
  const __FlashStringHelper* get_header_light();
  const __FlashStringHelper* get_menu();
  const __FlashStringHelper* get_body_begin();
  const __FlashStringHelper* get_body_end();
  
  void get_light_script(AcStorage * const lstorage, String * const str);
  
  void get_light_settings_mclock(String*, const RtcDateTime);
  const __FlashStringHelper* get_light_settings_mbegin();
  const __FlashStringHelper* get_light_settings_mend();
  
  void get_advset_light1(String*);
  void get_advset_light2(AcStorage * const lstorage, String * const str);
  void get_advset_light4(AcStorage * const lstorage, String *);
  
  void get_advset_light_device(unsigned int n_ldevices, AcStorage::deviceLightDescriptor *device, String * a);
  void get_advset_light_devicesel(AcStorage * const lstorage, String * const a);
  void get_advset_light_device_signals(AcStorage * const lstorage, String * a);
  void gen_listbox_lightdevices(AcStorage * const lstorage, String * const str);

  void get_advset_clock(String *, const RtcDateTime);
  void get_advset_psockets(String *str, unsigned int n_powerDevices, AcStorage::deviceDescriptorPW* pdevice);
  void get_advset_psockets_step( AcStorage * const lstorage, String * const str, ESP8266WebServer * server, std::function<void (String*)> sendBlock );

  void send_status_div(String * const str, AcStorage * const lstorage, std::function<void (String*)> sendBlock );
};

#endif // ACHTML_H

