
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

// definition 
ESP8266WebServer server ( 80 );

// real-time clock initialization
RtcDS3231 Rtc;

// Set up nRF24L01 radio on SPI bus plus pins CE=16 & CS=15
RF24 radio(16,15);

AcHtml html;
AcStorage storage;

String str;

OpenAq_Controller::OpenAq_Controller()
{ 
  //server = ESP8266WebServer(80);
  //parser = JsonParser<32>();
  //html = AcHtml();
  //storage = AcStorage();
  str.reserve(2048);
}


void OpenAq_Controller::setup_controller()
{
  delay(2000);
  
  factory_defaults(); // uncomment to set the factory defaults
  
  // setup serial for a baundrate of 115200
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  
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
  server.on("/advset", std::bind(&OpenAq_Controller::handleAdvset,this));
  server.on("/clock", std::bind(&OpenAq_Controller::handleClock,this));
  
  // setup webfiles for webserver
  for(int fl=0; fl < N_FILES; fl++)
  { 
    server.on(files[fl].filename, [=](){
      server.sendHeader("Content-Length", String(files[fl].len));
      server.send_P( 200, files[fl].mime, (PGM_P)files[fl].content, files[fl].len);
      
      //server.send(200, files[fl].mime, String(""));
      //server.sendContent_P(files[fl].content, files[fl].len);
      //server.client().stop();
      //server.send_P(200, files[fl].mime, (PGM_P)files[fl].content->access());

      /*int i;
      for(i=0; i < floor(files[fl].len/HTTP_DOWNLOAD_UNIT_SIZE); i++)
      {
        server.client().write(files[fl].content->access() + (i*HTTP_DOWNLOAD_UNIT_SIZE), HTTP_DOWNLOAD_UNIT_SIZE);
      }
      Serial.println(i);
      Serial.println((files[fl].len-(i*HTTP_DOWNLOAD_UNIT_SIZE)));
      server.client().write(files[fl].content->access() + (i*HTTP_DOWNLOAD_UNIT_SIZE), HTTP_DOWNLOAD_UNIT_SIZE);
      */
      
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
  //radio.printDetails();

  //for(int i=0; i<1024; i++)
  //  Serial.print(String(storage.arr[i])+" ");
}

unsigned int time_el=0;
void OpenAq_Controller::run_controller()
{
  server.handleClient();

  //for(int i=0; i<1024; i++)
  //  Serial.print(String(storage.arr[i])+" ");
  //Serial.println("");
  //delay(1000);
  //run_task_rf433ook();

  if(time_el >= 1000000)
  {
    run_task_ds3231();
    time_el = 0;
    Serial.println("Temp read...");
    run_task_nrf24();
  }
  else
  {
    time_el++;
  }
  
}

void OpenAq_Controller::run_task_rf433ook()
{
  static bool togle=false;
  
  // |-----------ID-----------||-GROUPFLAG-||-ON/OFF-||-GROUPID-||-DIMMER-|
  // 00111101111110001011111110    0           1         0000       0000
  // 00111101111110001011111110    0           1         0001       0000
  // lets sent a test message
  bool bvector[36] = {0,0,1,1,1,1,0,1,1,1,1,1,1,0,0,0,1,0,1,1,1,1,1,1,0,1, 0, togle, 0,0,0,0, 0,0,0,0}; // max message is 36bits
  
  for(int i=0; i<2; i++) {
    Rf433_transmitter.sendMessage(bvector);
    delay(10);
  }
    
  togle = !togle;
  
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

void OpenAq_Controller::factory_defaults()
{
  storage.defauls();
  storage.save();
}

// ##################
// ## PRIVATE PART ##
// ##################

void OpenAq_Controller::handleRoot()
{
  server.send(200, "text/html", String("<h1>OpenAq</h1> "));
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

  if ( server.hasArg("storesettings") )
  {
    storage.save();

    server.send(200, "text/html", String("<h3>Storage done</h3>"));
    
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

  // begin send blocks of data
  server.send(200, " text/html", String(""));
  //Serial.println("sending...");
  
  //String str = String("");
  // use external str
  str = "";
  
  str.concat(html.get_begin());
  str.concat(html.get_header());
  str.concat(html.get_body_begin());
  str.concat(html.get_menu());
  
  html.get_advset_light1(&str);
  sendBlock(&str);
  
  html.get_advset_light_device(storage.getNumberOfLightDevices(), storage.getLightDevices(), &str);
  sendBlock(&str);

  html.get_advset_light2(&str);
  
  html.get_advset_light_devicesel(storage.getNumberOfLightDevices(), storage.getLightDevices(), &str);
  sendBlock(&str);

  str.concat(html.get_advset_light3());
  
  html.get_advset_light_device_signals(storage.getNumberOfLightDevices(), storage.getLightDevices(), &str);
  sendBlock(&str);

  html.get_advset_light4(&str);
  sendBlock(&str);
  
  html.get_advset_clock(&str, clock);
  sendBlock(&str);

  html.get_advset_psockets(&str);
  sendBlock(&str);
  
  str.concat(html.get_body_end());
  str.concat(html.get_end());

  // send the remaining part
  sendBlock(&str);

  //Serial.println(str);
  server.client().write(str.c_str(), (str.length() % 4)==0? str.length() : str.length() + (4-(str.length()%4)));
  //server.send(200, "text/html", str);
  //server.client().stop();

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

OpenAq_Controller Controller;

