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

#include <pgmspace.h>
#include <FS.h>
#include <ESP8266httpUpdate.h>

#include "Opaq_c1.h"
#include "websites.h"

#include "Opaq_websockets.h"
#include "Opaq_storage.h"
#include "Opaq_iaqua.h"
#include "Opaq_iaqua_pages.h"

#if OPAQ_MDNS_RESPONDER
#include <ESP8266mDNS.h>
#endif

#if OPAQ_OTA_ARDUINO
#include <ArduinoOTA.h>
ArduinoOTA ota_server;
#endif

#ifdef OPAQ_C1_SCREEN
// change this....
#define TFT_CS 16
#define TFT_DC 15
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Opaq_iaqua iaqua;

ADS7846 touch;
LCD_HAL_Interface tft_interface = LCD_HAL_Interface(tft);
// until here...
#endif

// non-class functions begin

bool run1hz_flag = false;

static void ICACHE_FLASH_ATTR _deviceTaskLoop ( os_event_t* events )
{
  run1hz_flag = true;
}

static void ICACHE_FLASH_ATTR _10hzLoop ( os_event_t* events )
{

#ifdef OPAQ_C1_SCREEN
  opaq_controller.run_touch();
#endif

}

static void _devicesTask_timmingEvent()
{
  system_os_post ( deviceTaskPrio, 0, 0 );
}

static void _10hzLoop_timmingEvent()
{
  system_os_post ( _10hzLoopTaskPrio, 0, 0 );
}

// non-class functions end


/*=============================================================================
=                              Opaq public methods                            =
=============================================================================*/

OpenAq_Controller::OpenAq_Controller() :
  server ( AsyncWebServer ( 80 ) ),
  ws ( AsyncWebSocket("/ws") ),
  avrprog (328, 2),
  timming_events ( Ticker() ),
  //storage ( Opaq_storage() ),
  str ( String() ),
  clockIsReady( false )
{
  str.reserve ( 2048 );
}


void OpenAq_Controller::setup_controller()
{
  delay ( 100 );

  // initialize seed for code generation
  randomSeed ( ESP.getCycleCount() );

  // setup serial for a baundrate of 115200
  Serial.begin ( 115200 );
  Serial.setDebugOutput ( true );

  pinMode(5,OUTPUT); // AVR_CS OUT MODE
  digitalWrite(5, HIGH); // AVR_CS

  // Initialize file system.
  if (!SPIFFS.begin())
  {
    Serial.println(F("Failed to mount file system"));
    ESP.restart();
  }
  
  #ifdef OPAQ_C1_SCREEN
  // display welcome screen tft
  tft.begin();

  Opaq_iaqua_page_welcome wscreen = Opaq_iaqua_page_welcome();
  wscreen.draw();
  wscreen.setExecutionBar(5);
  #endif

  
  if ( storage.getSignature() != SIG )
  {
    Serial.println ( F("default settings will be applied.") );

#ifdef OPAQ_C1_SCREEN
    wscreen.setExecutionBar(100);
    wscreen.msg( String(F("Please reboot")).c_str() );
#endif

    storage.defaults ( SIG ); // uncomment to set the factory defaults
  }

  Serial.println("SIG Accepted");


  if ( storage.wifisett.getModeOperation() )
  {
    Serial.println("softAP");
    
    String ssid, pwd;
    storage.wifisett.getSSID(ssid);
    storage.wifisett.getPwd(pwd);

    Serial.print ( F("SSID: ") );
    Serial.println ( ssid );

#ifdef OPAQ_C1_SCREEN
    wscreen.setExecutionBar(20);
    wscreen.msg( String(F("Initializing AP mode")).c_str() );
#endif

    WiFi.mode(WIFI_AP);
    delay(5000);

#ifdef OPAQ_C1_SCREEN
    wscreen.setExecutionBar(45);
#endif
    
    WiFi.printDiag(Serial);

    // setup the access point
    WiFi.softAP ( ssid.c_str() , pwd.c_str(), 6 ); // with password and fixed ssid

    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
  }
  else
  {
    Serial.println("client");
    
    // or setup the station
    String ssid, pwd;
    
    storage.wifisett.getClientSSID(ssid);
    storage.wifisett.getClientPwd(pwd);

#ifdef OPAQ_C1_SCREEN
    wscreen.setExecutionBar(20);
    wscreen.msg( String(F("Initializing WIFI station")).c_str() );
#endif

    WiFi.mode(WIFI_STA);
    delay(5000);

#ifdef OPAQ_C1_SCREEN
    wscreen.setExecutionBar(45);
#endif

    Serial.print ( F("Connecting to ") );
    Serial.println ( ssid );

#ifdef OPAQ_C1_SCREEN
    wscreen.msg( (String(F("Connecting to ")) + ssid ).c_str() );
#endif

    WiFi.begin ( ssid.c_str(), pwd.c_str() );

    WiFi.printDiag(Serial);
    
    int count_tries = 0;

    while ( WiFi.status() != WL_CONNECTED )
    {
      delay ( 500 );
      Serial.print ( "." );
      count_tries++;

      if (count_tries > 100)
      {
        Serial.println ( F("returning to AP mode. Reboot.") );

#ifdef OPAQ_C1_SCREEN
        wscreen.msg( String(F("Returning to AP mode")).c_str() );
#endif

        delay(1000);
        storage.wifisett.enableSoftAP();
        ESP.reset();
      }
    }

#ifdef OPAQ_C1_SCREEN
    wscreen.msg( String(F("Connected")).c_str() );
#endif

    Serial.println ( F("WiFi connected") );
    Serial.println ( F("IP address: ") );
    Serial.println ( WiFi.localIP() );
  }

#ifdef OPAQ_C1_SCREEN
  wscreen.setExecutionBar(55);
#endif

#if OPAQ_OTA_ARDUINO
  // OTA server
  ota_server.setup();
#endif

#if OPAQ_MDNS_RESPONDER

#ifdef OPAQ_C1_SCREEN
  wscreen.msg( String(F("mDNS responder")).c_str() );
#endif

  // mDNS responder
  if (!MDNS.begin("opaq")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  MDNS.addService("http", "tcp", 80);
#endif

  //  BEGINS the LOADING OF FILES FROM SPIFFS

  auto serveron = [=](Dir d)
    {
      String filename = d.fileName();
      Serial.printf("%s %d  ", filename.c_str(), d.fileSize() );
      
      // get extension
      String ext = filename.substring(filename.indexOf('.', 0), filename.indexOf('\n', 0));
      const __FlashStringHelper* mime;
      
      if (ext == F(".txt"))
      {
        mime = F("text/plain");
      }
    
      Serial.println(mime);
      // THIS IS NOT THE SOLUTION FOR A MASSIVE AMOUNT OF FILES
      // [TODO]
      //if(ext == ".json") // puts the json files available for debugging
      //  server.on ( filename.c_str(), [=]() { server.send_F ( 200, String(mime).c_str(), filename.c_str() ); } ); // verify filename string (can desapear...)
    };
  
#ifdef OPAQ_C1_SCREEN
  wscreen.msg( String(F("Filesystem check")).c_str() );
#endif

  // search available SPIFFS files starting with prefix '_p'
  Dir d = SPIFFS.openDir("/");
  // list directory
  Serial.println("LIST DIR: ");
  while ( d.next() )
  {
    serveron(d);
  }

  //  ENDS the LOADING OF FILES FROM SPIFFS


  // setup webserver
  server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("opaqc1.html");

  server.onNotFound ( [ = ](AsyncWebServerRequest *request)
  {
    request->send ( 404, "  text/plain", String ( "    " ) );
  } );

  // attach AsyncWebSocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  

  // start server
  server.begin();

  // start avrprog
  avrprog.begin();

  // RTC setup
  //rtc.Begin();
  //Wire.begin ( 4, 5 );

#ifdef OPAQ_C1_SCREEN
  wscreen.setExecutionBar(75);

  // 
  // TOUCH CONTROLLER INITIALIZATION
  // 
  touch.begin();

  if ( !storage.touchsett.isTouchMatrixAvailable() )
  {
    // do calibration
    touch.doCalibration(&tft_interface);
    if( touch.getCalibrationMatrix(storage.touchsett.getTouchMatrixRef()) )
    {
      storage.touchsett.commitTouchSettings();
      Serial.println(F("Touch settings has been updated."));
    }
    else
    {
      Serial.println(F("Touch settings generation has been failed. Reseting ..."));
      
      ESP.reset();
    }
  }
  else
  {
    Serial.println(F("Touch settings has been read."));
    touch.setCalibration(storage.touchsett.getTouchMatrix());
  }
  // END TOUCH CONTROLLER INITIALIZATION


  wscreen.setExecutionBar(100);
#endif

  communicate.nrf24.init();

  // registers deviceTask in the OS control structures
  system_os_task ( _deviceTaskLoop, deviceTaskPrio, deviceTaskQueue,
                   deviceTaskQueueLen );

  system_os_task ( _10hzLoop, _10hzLoopTaskPrio, _10hzLoopQueue,
                   _10hzLoopTaskQueueLen );

  // Attach deviceTask event trigger function for periodic releases
  timming_events.attach_ms ( 1000, _devicesTask_timmingEvent );

  t_evt.attach_ms ( 50, _10hzLoop_timmingEvent );

#ifdef OPAQ_C1_SCREEN
  run_tft();
#endif
}

int count = 0;
bool opaq_defaults = false;
void OpenAq_Controller::run_controller()
{

  run_programmer();

  /*if (count == 100000)
  {
    //opaq_controller.run_task_rf433ook();
    count=0;
  }
  count++;*/

  if (storage.getUpdate())
  {
    // initialize opaq services
    storage.initOpaqC1Service();
    storage.setUpdate(false);
  }

  // run that at 1hz
  if(run1hz_flag)
  {
    run_task_ds3231();
    setClockReady();

    run_atsha204();

    storage.pwdevice.run();

    storage.faqdim.run();

    run1hz_flag = false;
  }

}

void OpenAq_Controller::run_programmer()
{
  static AVRISPState_t last_state = AVRISP_STATE_IDLE;
  bool lock = true;

  while(lock)
  {
    AVRISPState_t new_state = avrprog.update();
    if (last_state != new_state) {
        switch (new_state) {
            case AVRISP_STATE_IDLE: {
                Serial.printf("[AVRISP] now idle\r\n");
                lock = false;
                communicate.unlock();
                break;
            }
            case AVRISP_STATE_PENDING: {
                Serial.printf("[AVRISP] connection pending\r\n");
                // Clean up your other purposes and prepare for programming mode
                break;
            }
            case AVRISP_STATE_ACTIVE: {
                Serial.printf("[AVRISP] programming mode\r\n");
                // Stand by for completion
                communicate.spinlock();
                lock = true;
                break;
            }
        }
        last_state = new_state;
    }
    else
    {
      if(new_state == AVRISP_STATE_IDLE)
      {
        lock = false;
      }
    }

    // Serve the client
    if (last_state != AVRISP_STATE_IDLE)
    {
      avrprog.serve();
    }

    optimistic_yield(10000);
  }
}

uint16_t normalizeClock(RtcDateTime * clock, const uint16_t a, const uint16_t b )
{
  float clktmp = ( ( float )clock->Hour() ) + ( ( ( float )clock->Minute() ) / 60 )
                 + ( ( ( float )clock->Second() ) / 3600 );

  clktmp = (clktmp * 255.) / 24.;

  return (uint16_t)clktmp;
}

void OpenAq_Controller::run_task_ds3231()
{
  communicate.getClock(clock);
}

void OpenAq_Controller::run_atsha204()
{
  communicate.getCiferKey();
}

#ifdef OPAQ_C1_SCREEN
void OpenAq_Controller::run_touch()
{
  if( communicate.lock() )
    return;
    
  touch.service();
  
  /*Serial.println("ASK TOUCH:");
  Serial.println(touch.tp_x);
  Serial.println(touch.tp_y);
  Serial.println(touch.pressure);
  Serial.println(touch.getX());
  Serial.println(touch.getY());
  Serial.println("TOUCH ASKED!");
*/
  iaqua.service(touch.getX(),touch.getY(),touch.pressure);

  communicate.unlock();
  
}


void OpenAq_Controller::run_tft()
{
  Serial.println("ASK ILI9341");

  communicate.spinlock();
  
  tft.fillScreen(ILI9341_BLACK);
  iaqua.screenHome();

  communicate.unlock();
  
  Serial.println("ILI9341 ASKED");
  
}
#endif

/*--------------------  nrf24 device controller task  ---------------------*/

void OpenAq_Controller::syncClock()
{
  unsigned int localPort = 2390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

 Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
Serial.println(udp.localPort());

  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  //sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(timeServerIP, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
  // wait to see if a reply is available
  delay(1000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);


    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second


  // [TODO]
  RtcDateTime tmp = RtcDateTime( 0, 0, 0, (epoch  % 86400L) / 3600, (epoch  % 3600) / 60, epoch % 60);
  getCom().setClock(tmp);

}

}


/*======================  End of Opaq public methods  =======================*/




/*=============================================================================
=                      Opaq controller private handlers                       =
=============================================================================*/

/*
void sendBlockS(String* str, AsyncWebServer *sv)
{
  int index;
  for (index = 0; index < floor((*str).length() / HTTP_DOWNLOAD_UNIT_SIZE); index++)
  {
    sv->client().write((*str).c_str() + (index * HTTP_DOWNLOAD_UNIT_SIZE), HTTP_DOWNLOAD_UNIT_SIZE);
  }

  if ( index != 0 )
  {
    *str = (char *)((*str).c_str() + (index * HTTP_DOWNLOAD_UNIT_SIZE));
  }

  DEBUGV( "ac:: B %d", index );
}

void sendBlockCS(String* str, AsyncWebServer *sv, uint16_t* count)
{
  int index;
  for (index = 0; index < floor((*str).length() / HTTP_DOWNLOAD_UNIT_SIZE); index++)
  {
    *count += HTTP_DOWNLOAD_UNIT_SIZE;
  }

  if ( index != 0 )
  {
    *str = (char *)((*str).c_str() + (index * HTTP_DOWNLOAD_UNIT_SIZE));
  }

  DEBUGV( "ac:: CS %d", *count );
}

std::function<void(String*)> OpenAq_Controller::sendBlockGlobal( AsyncWebServer* sv, uint16_t* count, uint8_t* step )
{
  if ( *step )
  {
    return std::function<void(String*)>( [&sv]( String * str ) -> void { sendBlockS(str, sv); } );
  }
  else
  {
    return std::function<void(String*)>( [&sv, &count]( String * str ) -> void { sendBlockCS(str, sv, count); } );
  }
};

void OpenAq_Controller::handleRoot()
{
  AsyncWebServer *sv = &server;
  uint16_t count = 0;
  uint8_t step;

  // sends webpage content in chunks

  // first calculate the expected size of the webpage and then sends the webpage
  for ( step = 0; step < 2; step++ )
  {
    // reset global str
    str = "";

    if ( step == 1 )
    {
      server.setContentLength( count );
      server.send ( 200, "text/html", String ( "" ) );
    }

    str.concat ( html.get_begin() );
    str.concat ( html.get_header() );
    str.concat ( html.get_body_begin() );
    sendBlockGlobal( sv, &count, &step )( &str );

    str.concat ( html.get_menu() );

    html.send_status_div( &str, &storage, sendBlockGlobal(sv, &count, &step) );

    str.concat( html.get_body_end() );
    str.concat( html.get_end() );

    if ( step == 0 )
      count += str.length();
    else
      server.client().write( str.c_str(), str.length() );
  }
}

void zetlight_template1( uint8_t deviceId, Opaq_storage * const lstorage )
{
  auto apply_template = [&lstorage]( uint8_t deviceId, uint8_t signalId, uint8_t signal[SIGNAL_LENGTH][SIGNAL_STEP_SIZE], const uint8_t signal_len )
  {
    for (uint8_t i = 1; i <= SIGNAL_LENGTH; i++)
    {
      lstorage->setPointXLD( id2idx(deviceId), id2idx(signalId), i, signal[i - 1][0] );
      lstorage->setPointYLD( id2idx(deviceId), id2idx(signalId), i, signal[i - 1][1] );
    }

    lstorage->setLDeviceSignalLength( id2idx(deviceId), id2idx(signalId), signal_len );
  };

  uint8_t signal1[SIGNAL_LENGTH][SIGNAL_STEP_SIZE] = { {28, 0}, {63, 32}, {24, 79}, {0, 112}, {235, 157}, {242, 192},  {106, 233},  {28, 255},  {0, 0},  {0, 0},  {0, 0},  {0, 0},  {0, 0},  {0, 0},  {0, 0},  {0, 0} }; // number of steps 8
  uint8_t signal2[SIGNAL_LENGTH][SIGNAL_STEP_SIZE] = { {0, 0}, {0, 109}, {234, 155}, {244, 187}, {147, 206}, {0, 255}, {0, 0},  {0, 0}, {0, 0},  {0, 0},  {0, 0},  {0, 0},  {0, 0},  {0, 0},  {0, 0},  {0, 0} }; // number of steps 6

  apply_template( deviceId, 1, signal1, 8 );
  apply_template( deviceId, 2, signal2, 6 );
}

void OpenAq_Controller::handleLight()
{
  // lets send light configuration

  if (server.hasArg("adevice"))
  {
    storage.addLightDevice();

    server.send(200, "text/html", String(F("<h3>Add device done</h3>")));

    return;
  }

  if (server.hasArg("sigdev") && server.hasArg("asigid") && server.hasArg("asigpt") && server.hasArg("asigxy") && server.hasArg("asigvalue"))
  {
    uint8_t sig_dev = atoi(server.arg("sigdev").c_str());
    uint8_t sig_id  = atoi(server.arg("asigid").c_str());
    uint8_t sig_pt  = atoi(server.arg("asigpt").c_str());
    uint8_t sig_xy  = atoi(server.arg("asigxy").c_str());
    uint8_t sig_val = atoi(server.arg("asigvalue").c_str());

    DEBUGV("::ac %d %d %d %d %d\r\n", sig_dev, sig_id, sig_pt, sig_xy, sig_val);

    storage.addSignal(sig_dev, sig_id, sig_pt, sig_xy, sig_val);

    server.send(200, "text/html", String(F("<h3>Add signal done</h3>")));

    return;
  }

  if ( server.hasArg("sdevice") && server.hasArg("tdevice") )
  {
    uint8_t id   = atoi(server.arg("sdevice").c_str());
    uint8_t type = atoi(server.arg("tdevice").c_str());

    storage.setDeviceType(id, type);

    // set defaults when device is not initialized
    if ( !( 0 < storage.getLDeviceSignalLength(id2idx(id), 0) ) && type == ZETLIGHT_LANCIA_2CH )
    {
      zetlight_template1(id, &storage);
    }

    server.send(200, "text/html", String(F("<h3>Set device done</h3>")));

    return;
  }

  // update selected device
  if ( server.hasArg("sdevice") && server.hasArg("setcurrent") )
  {
    uint8_t id = atoi(server.arg("sdevice").c_str());
    storage.selectLDevice( id2idx(id) );
    server.send(200, "text/html", String(F("<h3>Device selected</h3>")));

    return;
  }

  if ( server.hasArg("sdevice") && server.hasArg("lstate") )
  {
    uint8_t idx = atoi(server.arg("sdevice").c_str());
    uint8_t state = atoi(server.arg("lstate").c_str());

    storage.setLState( idx, (pstate)state );
    server.send(200, "text/html", String(F("<h3>LDevice State Changed.</h3>")));

    return;
  }

  // default light webpage
  DEBUGV("ac:ihs: %s\r\n", String(ESP.getFreeHeap()).c_str());

  AsyncWebServer *sv = &server;
  uint16_t count = 0;
  uint8_t step;

  // first calculate the expected size of the webpage and then sends the webpage
  for ( step = 0; step < 2; step++ )
  {
    // reset global str
    str = String("");

    if ( step == 1 )
    {
      server.setContentLength( count );
      server.send ( 200, "text/html", String ( "" ) );
    }

    str += html.get_begin();
    str += html.get_header_light();
    str += html.get_body_begin();
    sendBlockGlobal(sv, &count, &step)(&str);
    str += html.get_menu();
    sendBlockGlobal(sv, &count, &step)(&str);

    html.get_light_script( &storage, &str, sendBlockGlobal( sv, &count, &step ) );
    str += html.get_light_settings_mbegin();
    sendBlockGlobal(sv, &count, &step)(&str);

    html.get_light_settings_mclock(&str, clock);
    sendBlockGlobal(sv, &count, &step)(&str);

    str += html.get_light_settings_mend();
    sendBlockGlobal(sv, &count, &step)(&str);

    //RtcTemperature temp = rtc.GetTemperature();
    str += String(
    //temp.AsWholeDegrees()
    0);
    str += ".";
    str += String(//temp.GetFractional()
    0);
    str += F("º</div>");
    sendBlockGlobal(sv, &count, &step)(&str);

    str += html.get_body_end();
    str += html.get_end();

    if ( step == 0 )
      count += str.length();
    else
      server.client().write( str.c_str(), str.length() );
  }

  Serial.println("L#Packet Lenght: " + String(str.length()));
  Serial.println("L#Heap Size: " + String(ESP.getFreeHeap()));

  return;
}

void OpenAq_Controller::handleAdvset()
{
  DEBUGV("ac:ihs: %s\r\n", String(ESP.getFreeHeap()).c_str());

  AsyncWebServer *sv = &server;
  uint16_t count = 0;
  uint8_t step;

  // first calculate the expected size of the webpage and then sends the webpage
  for ( step = 0; step < 2; step++ )
  {
    // reset global str
    str = "";

    if ( step == 1 )
    {
      server.setContentLength( count );
      server.send( 200, "text/html", String ( "" ) );
    }

    str.concat( html.get_begin() );
    str.concat( html.get_header() );
    str.concat( html.get_body_begin() );
    sendBlockGlobal( sv, &count, &step )( &str );
    str.concat( html.get_menu() );

    html.get_advset_light1( &str );
    sendBlockGlobal( sv, &count, &step )( &str );

    html.get_advset_light_device(storage.getNumberOfLightDevices(), storage.getLightDevices(), &str);
    sendBlockGlobal( sv, &count, &step )( &str );

    html.get_advset_light2(&storage, &str);

    html.get_advset_light_devicesel(&storage, &str);
    sendBlockGlobal( sv, &count, &step )( &str );

    html.get_advset_light_device_signals(&storage, &str);
    sendBlockGlobal( sv, &count, &step )( &str );

    html.get_advset_light4(&storage, &str);
    sendBlockGlobal( sv, &count, &step )( &str );

    html.get_advset_clock(&str, clock);
    sendBlockGlobal( sv, &count, &step )( &str );

    html.get_advset_psockets(&str, storage.getNumberOfPowerDevices(), storage.getPowerDevices());
    sendBlockGlobal( sv, &count, &step )( &str );

    html.get_advset_psockets_step( &storage, &str, &server, sendBlockGlobal( sv, &count, &step ) );
    DEBUGV("acH:: %d\n", str.length());
    sendBlockGlobal( sv, &count, &step )( &str );

    str.concat(html.get_body_end());
    str.concat(html.get_end());

    // send the remaining part
    sendBlockGlobal( sv, &count, &step )( &str );

    if ( step == 0 )
      count += str.length();
    else
      server.client().write( str.c_str(), str.length() );
  }

  Serial.println("Packet Lenght: " + String(str.length()));
  Serial.println("Heap Size: " + String(ESP.getFreeHeap()));
}

void OpenAq_Controller::handleClock()
{
  RtcDateTime dt;

  if (server.hasArg("stimeh"))
  {
    uint8_t h = atoi(server.arg("stimeh").c_str());
    dt = RtcDateTime(clock.Year(), clock.Month(), clock.Day(), h, clock.Minute(), clock.Second());
  }
  else if (server.hasArg("stimem"))
  {
    uint8_t m = atoi(server.arg("stimem").c_str());
    dt = RtcDateTime(clock.Year(), clock.Month(), clock.Day(), clock.Hour(), m, clock.Second());
  }
  else if (server.hasArg("stimed"))
  {
    uint8_t d = atoi(server.arg("stimed").c_str());
    dt = RtcDateTime(clock.Year(), clock.Month(), d, clock.Hour(), clock.Minute(), clock.Second());
  }
  else if (server.hasArg("stimemo"))
  {
    uint8_t mo = atoi(server.arg("stimemo").c_str());
    dt = RtcDateTime(clock.Year(), mo, clock.Day(), clock.Hour(), clock.Minute(), clock.Second());
  }
  else if (server.hasArg("stimey"))
  {
    uint16_t y = atoi(server.arg("stimey").c_str());
    dt = RtcDateTime(y, clock.Month(), clock.Day(), clock.Hour(), clock.Minute(), clock.Second());
  }
  else
    return;

  communicate.setClock(dt);

}

void OpenAq_Controller::handlePower()
{
  if (server.hasArg("addpdevice"))
  {
    storage.addPowerDevice();

    server.send(200, "text/html", String(F("<h3>Power device added</h3>")));

    return;
  }

  if ( server.hasArg("pdevice") && server.hasArg("pstate") )
  {
    uint8_t id = atoi(server.arg("pdevice").c_str());
    uint8_t state = atoi(server.arg("pstate").c_str());

    storage.setPowerDeviceState(id, state);

    if (state == ON)
    {
      server.send(200, "text/html", String(F("<h3>Power socket ON</h3>")));
    }
    else
    {
      if ( state == OFF )
      {
        server.send(200, "text/html", String(F("<h3>Power socket OFF</h3>")));
      }
      else
      {
        server.send(200, "text/html", String(F("<h3>Power socket BINDING</h3>")));
      }
    }

    return;
  }


  if ( server.hasArg("pdevice") && server.hasArg("psid") && server.hasArg("pvalue") )
  {
    uint8_t p_id = atoi(server.arg("pdevice").c_str());
    uint8_t step_id = atoi(server.arg("psid").c_str());
    uint8_t val = atoi(server.arg("pvalue").c_str());

    storage.setPDeviceStep( id2idx(p_id), id2idx(step_id), val );

    server.send(200, "text/html", String(F("<h3>Power socket value set</h3>")));

    return;
  }

  if ( server.hasArg("pdevice") && server.hasArg("pdesc") )
  {
    uint8_t p_id = atoi(server.arg("pdevice").c_str());

    storage.setPDesription( id2idx( p_id ), server.arg("pdesc").c_str() );

    server.send(200, "text/html", String(F("<h3>Power socket description set</h3>")));

    return;
  }

}

void OpenAq_Controller::handleGlobal()
{
  if ( server.hasArg("storesettings") )
  {
    storage.save();

    server.send(200, "text/html", String(F("Settings are stored")));

    return;
  }

  if ( server.hasArg("clientssid") && server.hasArg("clientpwd") )
  {
    storage.setClientSSID( server.arg("clientssid").c_str() );
    storage.setClientPwd( server.arg("clientpwd").c_str() );

    storage.enableClient();

    server.send(200, "text/html", String(F("<h3>Client enabled.</h3>")));
    
    storage.save();

    WiFi.mode(WIFI_STA);
    delay(5000);
    WiFi.begin ( storage.getClientSSID(), storage.getClientPwd() );
    
    ESP.reset();

    return;
  }

  if ( server.hasArg("setfactorysettings") )
  {
    storage.defaults(SIG);
    //storage.save();
    //ESP.restart();
  }

  if ( server.hasArg("setreboot") )
  {
    ESP.restart();
  }

  if ( server.hasArg("update") )
  {
    ota();
  }

  if ( server.hasArg("opaqdefaults") )
  {
    server.send(200, "text/html", String(F("<h3>OPAQ_DEFAULTS_RUNNING</h3>")));
    opaq_defaults = true;
  }
}


void OpenAq_Controller::ota()
{

  t_httpUpdate_return ret = ESPhttpUpdate.update(OPAQ_URL_FIRMWARE_UPLOAD, 80, "/binaries", OPAQ_VERSION);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.println("HTTP_UPDATE_FAILED");
      server.send(200, "text/html", String(F("<h3>HTTP_UPDATE_FAILED</h3>")));
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

  }
}
*/
/*=====  End of Opaq controller private methods  ======*/

OpenAq_Controller opaq_controller;

