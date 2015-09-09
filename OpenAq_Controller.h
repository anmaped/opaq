
#ifndef OPENAQ_H
#define OPENAQ_H

#include "AcHtml.h"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <JsonParser.h>

#include <RtcDS3231.h>

class OpenAq_Controller
{
private:
  
  //ESP8266WebServer server;
  //JsonParser<32> parser;
  RtcDateTime clock;
  
  void handleRoot();
  void handleLight();
  void handleAdvset();
  void handleClock();
  void handlePower();
  void handleGlobal();

public:

  OpenAq_Controller();

  void factory_defaults(uint8_t sig);
  void setup_controller();
  
  void run_controller();
  void run_task_rf433ook();
  void run_task_ds3231();
  void run_task_nrf24();
  
  void sendBlock(String *str);
  
};

extern OpenAq_Controller Controller;

#endif // OPENAQ_H
