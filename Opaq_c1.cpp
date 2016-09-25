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
#include "opaq.h"

#include "Opaq_websockets.h"
#include "Opaq_storage.h"
#include "Opaq_iaqua.h"
#include "Opaq_iaqua_pages.h"
#include "Opaq_recovery.h"
#include "Opaq_command.h"

#include "slre.h"

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

LCD_HAL_Interface tft_interface = LCD_HAL_Interface(tft);
// until here...
#endif


OpenAq_Controller opaq_controller;

// non-class functions begin

bool run1hz_flag = false;
bool enableTartExtract = false;

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

  // reovery mode
  byte _c;
  delay(1000);
  if(Serial.readBytes(&_c,1))
  {
    Serial.println("recovery mode!");
    return;
  }

  // Initialize file system.
  if (!SPIFFS.begin())
  {
    Serial.println(F("Failed to mount file system"));
    ESP.restart();
  }

  Serial.println(String(F("Opaq Version ")) + OPAQ_VERSION);
  Serial.println(F("Copyright (c) 2015 Andre Pedro. All rights reserved."));
  
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

  Serial.println(F("SIG Accepted"));


  if ( storage.wifisett.getModeOperation() )
  {
    Serial.println(F("softAP"));
    
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
    delay(4000);

#ifdef OPAQ_C1_SCREEN
    wscreen.setExecutionBar(45);
#endif

    // setup the access point
    WiFi.softAP ( ssid.c_str() , pwd.c_str(), 6 ); // with password and fixed ssid

    Serial.print(F("IP address: "));
    Serial.println(WiFi.softAPIP());
  }
  else
  {
    Serial.println(F("client"));
    
    // or setup the station
    String ssid, pwd;
    
    storage.wifisett.getClientSSID(ssid);
    storage.wifisett.getClientPwd(pwd);

#ifdef OPAQ_C1_SCREEN
    wscreen.setExecutionBar(20);
    wscreen.msg( String(F("Initializing WIFI station")).c_str() );
#endif

    WiFi.mode(WIFI_STA);
    delay(4000);

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
    Serial.println(F("Error setting up MDNS responder!"));
    while (1) {
      delay(1000);
    }
  }
  Serial.println(F("mDNS responder started"));
  MDNS.addService("http", "tcp", 80);
#endif

  //  BEGINS the LOADING OF FILES FROM SPIFFS
  
#ifdef OPAQ_C1_SCREEN
  wscreen.msg( String(F("Filesystem check")).c_str() );
#endif


  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200);
  }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){

      static File f;

      /*// filename and path to store
      char * filename[64] = "foo";

      if(request->hasParam("filename", true))
      {
        AsyncWebParameter* p = request->getParam("filename", true);
        p->name().c_str()
        strcpy(filename, p->value().c_str());
      }*/

      if(!index){

        // test if we want to format
        //SPIFFS.format();

        if(!f)
        {
          f = SPIFFS.open(String(String(F("/tmp/")) + filename).c_str(), "w");
        }

        Serial.printf("UploadStart: %s\n", filename.c_str());
      }

      for(size_t i=0; i<len; i++){
        f.write(data[i]);
      }

      if(final){
        f.close();

        struct slre_cap caps[5];

        // ex: "/opaqc1-www-v001.tar"
        String lre_tar = String(F("^opaqc1-([a-z]+)-v([0-9]+).tar$"));
        // ex: "/opaqc1-v001-MD5.bin"
        String lre_bin = String(F("^opaqc1-v([0-9]+)-([a-z0-9]+).bin$"));
        // ex: "/opaqc1-avr-v001-MD5.bin"
        String lre_avr_bin = String(F("^opaqc1-avr-v([0-9]+)-([a-z0-9]+).bin$"));

        // contain tar extension ? apply tar extractor.
        if (slre_match(lre_tar.c_str(), filename.c_str(), filename.length(), caps, 5, 0) > 0)
        //if(filename == "www.tar")
        {
          String target = F("/");
          target += caps[0].ptr;
          target.setCharAt(caps[0].len + 1, '\0');
          Serial.printf("Path: %s\n", target.c_str());

          oq_cmd c;
          c.exec = [](LinkedList<String>& args) { storage.tarextract(args.pop().c_str(), args.pop().c_str()); };

          String filepath = String(F("/tmp/")) + filename;

          c.args.add(target);
          c.args.add(filepath);

          command.send(c);

          request->redirect("/rcv?success=true");
        }
        else
          if (slre_match(lre_bin.c_str(), filename.c_str(), filename.length(), caps, 5, 0) > 0)
          //if(filename == "fw.bin")
          {
            String md5 = "";
            md5 += caps[1].ptr;
            md5.setCharAt(caps[1].len, '\0');
            Serial.printf("MD5: %s\n", md5.c_str());

            oq_cmd c;
            c.exec = [](LinkedList<String> args) { storage.fwupdate(args.pop().c_str(), args.pop().c_str()); };
            c.args = LinkedList<String>();
            String filepath = String(F("/tmp/")) + filename;

            c.args.add(md5);
            c.args.add(filepath);

            command.send(c);

            request->redirect("/rcv?success=true&fw=true");
          }
          else
            if(slre_match(lre_avr_bin.c_str(), filename.c_str(), filename.length(), caps, 5, 0) > 0)
            {
              storage.avrprog.program(filename.c_str());
            }
            else
            {
              //request->send(200, "text/html", "SUCCESS.");
              request->redirect("/rcv?success=false");
            }

        Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index+len);

      }
    }
  );

  // Simple Firmware Update Form
  server.on("/rcv", HTTP_GET, [](AsyncWebServerRequest *request){
    opaq_recovery(request);
    //request->send(200, "text/html","<form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='upload'><input type='submit' value='upload'></form><a href='format'>Format</a>");
  });

  server.on("/formatspiffs", HTTP_GET, [](AsyncWebServerRequest *request){
    
    oq_cmd c;
    c.exec = [](LinkedList<String> args) { SPIFFS.format(); };
    c.args = LinkedList<String>();

    command.send(c);

    request->send(200);
  });

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
  storage.avrprog.begin();

  // RTC setup
  //rtc.Begin();
  //Wire.begin ( 4, 5 );

#ifdef OPAQ_C1_SCREEN
  wscreen.setExecutionBar(75);

  // 
  // TOUCH CONTROLLER INITIALIZATION
  // 
  communicate.touch.begin();

  if ( !storage.touchsett.isTouchMatrixAvailable() )
  {
    // do calibration
    communicate.touch.doCalibration(tft_interface);
    if( communicate.touch.getCalibrationMatrix(storage.touchsett.getTouchMatrixRef()) )
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
    communicate.touch.setCalibration(storage.touchsett.getTouchMatrix());
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

  Serial.print(F("opaq>"));

}

void OpenAq_Controller::run_controller()
{
  // program handler
  run_programmer();

  if (storage.getUpdate())
  {
    // initialize opaq services
    storage.initOpaqC1Service();
    storage.setUpdate(false);
  }

  command.handler();

  // run that at 1hz
  if(run1hz_flag)
  {
    run_task_ds3231();
    setClockReady();

    communicate.atsha204.getCiferKey();

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
    AVRISPState_t new_state = storage.avrprog.update();
    if (last_state != new_state) {
        switch (new_state) {
            case AVRISP_STATE_IDLE: {
                Serial.println(F("[AVRISP] now idle"));
                lock = false;
                communicate.unlock();
                break;
            }
            case AVRISP_STATE_PENDING: {
                Serial.println(F("[AVRISP] connection pending"));
                // Clean up your other purposes and prepare for programming mode
                break;
            }
            case AVRISP_STATE_ACTIVE: {
                Serial.println(F("[AVRISP] programming mode"));
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
      storage.avrprog.serve();
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

#ifdef OPAQ_C1_SCREEN
void OpenAq_Controller::run_touch()
{
  communicate.touch.service();
  touch_t data = communicate.touch.get();

  if( communicate.lock() )
    return;

  iaqua.service(data.x, data.y, data.pressure);

  communicate.unlock();
  
}


void OpenAq_Controller::run_tft()
{
  //Serial.println(F("ASK ILI9341"));

  communicate.spinlock();
  
  tft.fillScreen(ILI9341_BLACK);
  iaqua.screenHome();

  communicate.unlock();
  
  //Serial.println(F("ILI9341 ASKED"));
  
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
