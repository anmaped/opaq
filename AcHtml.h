
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

  void get_advset_clock(String *, const RtcDateTime);
  void get_advset_psockets(String *str, unsigned int n_powerDevices, AcStorage::deviceDescriptorPW* pdevice);
  void get_advset_psockets_step( AcStorage * const lstorage, String * const str, ESP8266WebServer * server );
};

#endif // ACHTML_H

