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

#include "Opaq_storage.h"

#include <String.h>

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <RtcDateTime.h>


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
  
  void get_light_script(Opaq_storage * const lstorage, String * const str, std::function<void (String*)> sendBlock);
  
  void get_light_settings_mclock(String*, const RtcDateTime);
  const __FlashStringHelper* get_light_settings_mbegin();
  const __FlashStringHelper* get_light_settings_mend();
  
  void get_advset_light1(String*);
  void get_advset_light2(Opaq_storage * const lstorage, String * const str);
  void get_advset_light4(Opaq_storage * const lstorage, String *);
  
  void get_advset_light_device(unsigned int n_ldevices, Opaq_storage::deviceLightDescriptor *device, String * a);
  void get_advset_light_devicesel(Opaq_storage * const lstorage, String * const a);
  void get_advset_light_device_signals(Opaq_storage * const lstorage, String * a);
  void gen_listbox_lightdevices(Opaq_storage * const lstorage, String * const str);

  void get_advset_clock(String *, const RtcDateTime);
  void get_advset_psockets(String *str, unsigned int n_powerDevices, Opaq_storage::deviceDescriptorPW* pdevice);
  void get_advset_psockets_step( Opaq_storage * const lstorage, String * const str, AsyncWebServer * server, std::function<void (String*)> sendBlock );

  void send_status_div(String * const str, Opaq_storage * const lstorage, std::function<void (String*)> sendBlock );
};

#endif // ACHTML_H

