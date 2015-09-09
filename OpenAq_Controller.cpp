
#include "OpenAq_Controller.h"
#include "Rf433ook.h"

#include "websites.h"

#include <pgmspace.h>

#include <ESP8266WiFi.h> 
#include <ESP8266WebServer.h>
#include <JsonParser.h>

#include <RtcDS3231.h>

#include <nRF24L01.h>
#include <RF24.h>

#include <Wire.h>
#include <EEPROM.h>
#include <Ticker.h>

extern "C" {
#include "c_types.h"
#include "eagle_soc.h"
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
}

#define SIG 0x56

// definition 
ESP8266WebServer server ( 80 );

// real-time clock initialization
RtcDS3231 Rtc;

// Set up nRF24L01 radio on SPI bus plus pins CE=16 & CS=15
RF24 radio(16,15);

AcHtml html;
AcStorage storage;

Ticker tk;

#define user_procTaskPrio        1
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];

static void ICACHE_FLASH_ATTR user_procTask(os_event_t *events)
{
    Serial.println("ONLYONE!!");

    Controller.run_task_ds3231();
    Controller.run_task_nrf24();
    Controller.run_task_rf433ook();
    
}

static void testfun()
{
  Serial.println("ON!");
  system_os_post(user_procTaskPrio, 0, 0 );
}

static String str;

OpenAq_Controller::OpenAq_Controller()
{ 
  str.reserve(2048);
}


void OpenAq_Controller::setup_controller()
{
  delay(2000);

  // initialize seed for code generation
  randomSeed(ESP.getCycleCount());
  
  // setup serial for a baundrate of 115200
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  if ( storage.getSignature() != SIG )
  {
    factory_defaults(SIG); // uncomment to set the factory defaults
    Serial.println("default settings applied.");
  }

  
  // read openaq mode from eeprom
  bool mode = storage.getModeOperation() & 0b00000001;
  
  if (mode)
  {
    const char *ssid = storage.getSSID();
    
    Serial.print("SSID: ");
    Serial.println(ssid);
    
    // setup the access point
    WiFi.softAP(ssid); // now without password and fixed ssid [TODO]
  }
  else
  {
    // or setup the station
    
  }
  
  // setup webserver
  server.on("/", std::bind(&OpenAq_Controller::handleRoot,this));
  server.on("/light", std::bind(&OpenAq_Controller::handleLight,this));
  server.on("/clock", std::bind(&OpenAq_Controller::handleClock,this));
  server.on("/power", std::bind(&OpenAq_Controller::handlePower,this));
  server.on("/advset", std::bind(&OpenAq_Controller::handleAdvset,this));
  server.on("/global", std::bind(&OpenAq_Controller::handleGlobal,this));
  
  // setup webfiles for webserver
  for(int fl=0; fl < N_FILES; fl++)
  { 
    server.on(files[fl].filename, [=](){
      server.sendHeader("Content-Length", String(files[fl].len));
      server.send_P( 200, files[fl].mime, (PGM_P)files[fl].content, files[fl].len);
    });
  }
  
  server.onNotFound ( [=](){
    server.send ( 404, "  text/plain", String("    ") );
    } );
  
  // start server
  server.begin();
  
  // RF433 setup 
  Rf433_transmitter.set_pin(2);
  Rf433_transmitter.set_encoding(Rf433_transmitter.CHANON_DIO_DEVICE);
  
  // RTC setup
  Rtc.Begin();
  Wire.begin(4, 5);
  
  // NRF24 setup and radio configuration
  radio.begin();
  //radio.setChannel(1);
  //radio.setDataRate(RF24_2MBPS);
  
  // configure pipe 0x1122334455 for NRF24
  //radio.openWritingPipe(0x1122334455LL);
  
  // manual test
  uint8_t buf[5] = {0x11,0x22,0x33,0x44,0x55};
  radio.write_register(TX_ADDR, buf, 5);
  radio.write_register(SETUP_AW, 0x3 );
  radio.write_register(EN_AA, 0x0 );
  radio.write_register(EN_RXADDR, 0x0 );
  radio.write_register(SETUP_RETR, 0x0 );
  radio.write_register(RF_CH, 0x1 );
  radio.write_register(RF_SETUP, 0x7 );
  radio.write_register(CONFIG, 0xE );
  
  // for debug purposes
  radio.printDetails();

  //
  //static volatile os_timer_t network_timer;
  //os_timer_arm(&network_timer, 10000, 1);
  tk.attach_ms(1000, testfun);

  system_os_task(user_procTask, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
}

void OpenAq_Controller::run_controller()
{
  server.handleClient();
  
}

void doState(uint8_t pdeviceId)
{
  bool vector[36] = { 0 };
    // vector is organized as follows :
    // |-----------ID-----------||-GROUPFLAG-||-ON/OFF-||-GROUPID-||-DIMMER-|
    // 00111101111110001011111110    0           1         0000       0000
    // 00111101111110001011111110    0           1         0001       0000
    
    int j=0;
    uint32_t code = storage.getPDeviceCode(pdeviceId);
    
    while ( j < 26 && code )
    {
      vector[j++] = code & 1;
      code >>= 1;
    }

    // set group flag
    vector[26] = 0;

    // set state
    uint8_t state = storage.getPDeviceState(pdeviceId);
    if ( state == OFF || state == UNBINDING )
    {
      vector[27] = 0;
    }
    else
    {
      vector[27] = 1;
    }

    for ( int i=0; i<36; i++)
      DEBUGV("%d,",vector[i]);
    DEBUGV("\n");
    // lets sent a test message
    //bool bvector[36] = {1,1,0,0,0,0,1,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,1,0, 0, vector[27], 0,0,0,0, 0,0,0,0}; // max message is 36bits
    
    for(int i=0; i<2; i++) {
      Rf433_transmitter.sendMessage(vector);
      delayMicroseconds(10000);
    }
}

void OpenAq_Controller::run_task_rf433ook()
{ 
  // check if some device is binding...
  for ( int pdeviceId = 1; pdeviceId <= storage.getNumberOfPowerDevices(); pdeviceId++ )
  {
    if (storage.getPDeviceState(pdeviceId) == BINDING)
    {
      // do binding

      doState(pdeviceId);

      return;
    }
  }

  // check if some device is unbinding
  for ( int pdeviceId = 1; pdeviceId <= storage.getNumberOfPowerDevices(); pdeviceId++ )
  {
    if (storage.getPDeviceState(pdeviceId) == UNBINDING)
    {
      // do unbinding

      doState(pdeviceId);

      return;
    }
  }
  

  // normal execution
  for ( int pdeviceId = 1; pdeviceId <= storage.getNumberOfPowerDevices(); pdeviceId++ )
  {
    DEBUGV("ac:: power %d\r\n", storage.getPDeviceState(pdeviceId));
    
    doState(pdeviceId);
  }
}

void OpenAq_Controller::run_task_ds3231()
{
  // update clock
  clock = Rtc.GetDateTime();
}

void OpenAq_Controller::run_task_nrf24()
{
  float clktmp;
  
  for ( int deviceId = 1; deviceId <= storage.getNumberOfLightDevices(); deviceId++ )
  {
    // case device is 
    if (storage.getDeviceType(deviceId) == ZETLIGHT_LANCIA_2CH)
    {
      // compute signal values according to the clock
      uint8_t signal1=0;
      uint8_t signal2=0;

      clktmp = ((float)clock.Hour()) + (((float)clock.Minute())/60) + (((float)clock.Second())/3600);
      storage.getLinearInterpolatedPoint(deviceId, 1, clktmp, &signal1);
      storage.getLinearInterpolatedPoint(deviceId, 2, clktmp, &signal2);
      
      //                                       <-     LIGTH     ->                                               <LRC>
      //                                  <Mode>            <Mode2>
      uint8_t buf[32] = {0xEE,0xD,0xA,    0x03,      signal1,    0x00,     signal2,   0x10,0x0,0x0,0x0,0x0,0x0,0x0,      0x00  ,    0xEC,0x0,0x0,0x0,0xF2,0x83,0x1D,0x4A,0x17,0x0,0x0,0x68,0x71,0x0,0x0,0x0,0x0};
      //  <Mode>= DAM(0x03); SUNRISE(0xb6); DAYTIME(0xb6); SUNSET(0xD4); NIGHTTIME(0x06)
      //  <Mode2>= DAM(0x00); SUNRISE(0x15); DAYTIME(0x58); SUNSET(0x2A); NIGHTIME(0x2A)
      
      // calculate LRC - longitudinal redundancy check
      uint8_t LRC = 0;
      for (int j = 1; j < 14; j++)
      {
          LRC ^= buf[j];
      }
      
      buf[14] = LRC;
      
      // send message
      
      radio.startWrite(buf, 32);
      
      //radio.printDetails();
    }
  }
  
}

void OpenAq_Controller::factory_defaults(uint8_t sig)
{
  storage.defauls(sig);
  storage.save();
}

// ##################
// ## PRIVATE PART ##
// ##################

void OpenAq_Controller::handleRoot()
{
   // begin sending webpage content in blocks
  server.send( 200, " text/html", String("") );
  
  // use global str
  str = "";
  
  str.concat( html.get_begin() );
  str.concat( html.get_header() );
  str.concat( html.get_body_begin() );
  str.concat( html.get_menu() );

static const char ss_tmp[] PROGMEM = R"=====(
<style>
.setting {
  position:relative;
  margin-bottom:.453em;
  line-height:1.993em;
  width:100%;
}
.setting .label {
  float:left;
  width:18.931em;
}
fieldset .setting .label {
  width:18.116em;
}
.setting .default {
  display:block;
  position:absolute;
  color:#666;
  top:0;
  right:0;
  width:16.304em;
}
fieldset {
  border:solid .09em #ccc;
  padding:.725em;
  margin:0;
}
fieldset legend {
  font-weight:bold;
  margin-left:-.362em;
  padding:0 .272em;
  background:#fff;
}
</style>
<div class="settings" id="settings"><h2><a href="#">System Information</a></h2>
  <fieldset>
<legend>Controller</legend>
<div class="setting">
  <div class="label">Model</div>
  Opaq v1.0&nbsp;
</div>
<div class="setting">
  <div class="label">Wireless MAC</div>
  <span id="wl_mac" style="cursor:pointer; text-decoration:underline;" title="OUI Search" onclick="getOUIFromMAC(&#39;00:14:BF:3B:F8:DD&#39;)">00:14:BF:3B:F8:DD</span>&nbsp;
</div>
</fieldset><br>

<fieldset>
<legend>Wireless</legend>
<div class="setting">
<div class="label">Radio Time Restrictions</div>
<span id="wl_radio">Radio is On</span>&nbsp;
</div>
<div class="setting">
<div class="label">Mode</div>
AP&nbsp;
&nbsp;
</div>
<div class="setting">
<div class="label">SSID</div>
opaq-AAAA&nbsp;
</div>
<div class="setting">
<div class="label">DHCP Server</div>
Enabled&nbsp;
</div>
<div class="setting">
<div class="label">Channel</div>
<span id="wl_channel">6</span>&nbsp;
</div>
<div class="setting">
<div class="label">Rate</div>
<span id="wl_rate">54 Mbps</span>&nbsp;
</div>
</fieldset><br>
</div>
)=====";

  str += FPSTR(&ss_tmp[0]);
  sendBlock(&str);

  str.concat(html.get_body_end());
  str.concat(html.get_end());

  server.client().write( str.c_str(), str.length() );
}

void OpenAq_Controller::handleLight()
{
  // lets send light configuration
  
  if(server.hasArg("adevice"))
  {
    storage.addLightDevice();
    
    server.send(200, "text/html", String("<h3>Add device done</h3>"));
    
    return;
  }
  
  if(server.hasArg("sigdev") && server.hasArg("asigid") && server.hasArg("asigpt") && server.hasArg("asigxy") && server.hasArg("asigvalue"))
  {
    uint8_t sig_dev = atoi(server.arg("sigdev").c_str());
    uint8_t sig_id = atoi(server.arg("asigid").c_str());
    uint8_t sig_pt = atoi(server.arg("asigpt").c_str());
    uint8_t sig_xy = atoi(server.arg("asigxy").c_str());
    uint8_t sig_val = atoi(server.arg("asigvalue").c_str());

    DEBUGV("::ac %d %d %d %d %d\r\n", sig_dev, sig_id, sig_pt, sig_xy, sig_val);
    
    storage.addSignal(sig_dev, sig_id, sig_pt, sig_xy, sig_val);

    server.send(200, "text/html", String("<h3>Add signal done</h3>"));
    
    return;
  }

  if ( server.hasArg("sdevice") && server.hasArg("tdevice") )
  {
    uint8_t id = atoi(server.arg("sdevice").c_str());
    uint8_t type = atoi(server.arg("tdevice").c_str());
    storage.setDeviceType(id, type);

    server.send(200, "text/html", String("<h3>Set device done</h3>"));
    
    return;
  }

  /* update selected device */
  if ( server.hasArg("sdevice") && server.hasArg("setcurrent") )
  {
    uint8_t id = atoi(server.arg("sdevice").c_str());
    storage.selectLDevice(id2idx(id));
    server.send(200, "text/html", String("<h3>Device selected</h3>"));
    
    return;
  }
  
  // default light webpage
  DEBUGV("ac:ihs: %s\r\n", String(ESP.getFreeHeap()).c_str());

  // begin send blocks of data
  server.send(200, "   text/html", String(""));
  
  //String str = String("");
  // use external str
  str = "";
  
  str += html.get_begin();
  str += html.get_header_light();
  str += html.get_body_begin();
  str += html.get_menu();
  sendBlock(&str);
  
  html.get_light_script(&str, storage.getNumberOfLightDevices(), storage.getLightDevices());
  str += html.get_light_settings_mbegin();
  sendBlock(&str);
  
  html.get_light_settings_mclock(&str, clock);
  sendBlock(&str);
  
  str += html.get_light_settings_mend();
  sendBlock(&str);
  
  RtcTemperature temp = Rtc.GetTemperature();
  str += String(temp.AsWholeDegrees());
  str += ".";
  str += String(temp.GetFractional());
  str += F("º</div>");
  sendBlock(&str);
  
  str += html.get_body_end();
  str += html.get_end();

  server.client().write(str.c_str(), (str.length() % 4)==0? str.length() : str.length() + (4-(str.length()%4)));
  //server.send(200, "text/html", str.c_str());
  
  Serial.println("L#Packet Lenght: " + String(str.length()));
  Serial.println("L#Heap Size: " + String(ESP.getFreeHeap()));
  
  return;
}

void OpenAq_Controller::sendBlock(String *str)
{
  int index;
  for (index=0; index < floor((*str).length()/HTTP_DOWNLOAD_UNIT_SIZE); index++)
  {
    server.client().write((*str).c_str() + (index * HTTP_DOWNLOAD_UNIT_SIZE), HTTP_DOWNLOAD_UNIT_SIZE);
  }
  
  if ( index != 0 )
  {
    *str = ((*str).c_str() + (index * HTTP_DOWNLOAD_UNIT_SIZE));
  }

  Serial.println("Block sended: " + String(index));
}

void OpenAq_Controller::handleAdvset()
{
  DEBUGV("ac:ihs: %s\r\n", String(ESP.getFreeHeap()).c_str());

  // BEGIN: send blocks of data
  server.send( 200, " text/html", String("") );
  
  // use global str
  str = "";
  
  str.concat( html.get_begin() );
  str.concat( html.get_header() );
  str.concat( html.get_body_begin() );
  str.concat( html.get_menu() );
  
  html.get_advset_light1( &str );
  sendBlock( &str );
  
  html.get_advset_light_device(storage.getNumberOfLightDevices(), storage.getLightDevices(), &str);
  sendBlock(&str);

  html.get_advset_light2(&storage, &str);
  
  html.get_advset_light_devicesel(&storage, &str);
  sendBlock(&str);
  
  html.get_advset_light_device_signals(&storage, &str);
  sendBlock(&str);

  html.get_advset_light4(&storage, &str);
  sendBlock(&str);
  
  html.get_advset_clock(&str, clock);
  sendBlock(&str);

  html.get_advset_psockets(&str, storage.getNumberOfPowerDevices(), storage.getPowerDevices());
  sendBlock(&str);
  
  str.concat(html.get_body_end());
  str.concat(html.get_end());

  // send the remaining part
  sendBlock(&str);

  //Serial.println(str);
  server.client().write( str.c_str(), str.length() );

  Serial.println("Packet Lenght: " + String(str.length()));
  Serial.println("Heap Size: " + String(ESP.getFreeHeap()));
}

void OpenAq_Controller::handleClock()
{
  RtcDateTime dt;
  
  if(server.hasArg("stimeh"))
  {
    uint8_t h = atoi(server.arg("stimeh").c_str());
    dt = RtcDateTime(clock.Year(), clock.Month(), clock.Day(), h, clock.Minute(), clock.Second()); 
  }
  else
    if(server.hasArg("stimem"))
    {
      uint8_t m = atoi(server.arg("stimem").c_str());
      dt = RtcDateTime(clock.Year(), clock.Month(), clock.Day(), clock.Hour(), m, clock.Second()); 
    }
    else
      if(server.hasArg("stimed"))
      {
        uint8_t d = atoi(server.arg("stimed").c_str());
        dt = RtcDateTime(clock.Year(), clock.Month(), d, clock.Hour(), clock.Minute(), clock.Second()); 
      }
      else
        if(server.hasArg("stimemo"))
        {
          uint8_t mo = atoi(server.arg("stimemo").c_str());
        dt = RtcDateTime(clock.Year(), mo, clock.Day(), clock.Hour(), clock.Minute(), clock.Second()); 
        }
        else
          if(server.hasArg("stimey"))
          {
            uint16_t y = atoi(server.arg("stimey").c_str());
            dt = RtcDateTime(y, clock.Month(), clock.Day(), clock.Hour(), clock.Minute(), clock.Second()); 
          }
          else
            return;

  Rtc.SetDateTime(dt);
  
}

void OpenAq_Controller::handlePower()
{
  if (server.hasArg("addpdevice"))
  {
    storage.addPowerDevice();
    
    server.send(200, "text/html", String("<h3>Power device added</h3>"));
    
    return;
  }
  
  if ( server.hasArg("pdevice") && server.hasArg("pstate") )
  {
    uint8_t id = atoi(server.arg("pdevice").c_str());
    uint8_t state = atoi(server.arg("pstate").c_str());
    
    storage.setPowerDeviceState(id, state);
    
    if (state == ON)
    {
      server.send(200, "text/html", String("<h3>Power socket ON</h3>"));
    }
    else
    {
      if ( state == OFF )
      {
        server.send(200, "text/html", String("<h3>Power socket OFF</h3>"));
      }
      else
      {
        server.send(200, "text/html", String("<h3>Power socket BINDING</h3>"));
      }
    }
    
    return;
  }
}

void OpenAq_Controller::handleGlobal()
{
  if ( server.hasArg("storesettings") )
  {
    storage.save();

    server.send(200, "text/html", String("<h3>Storage done</h3>"));
    
    return;
  }
}

OpenAq_Controller Controller;

