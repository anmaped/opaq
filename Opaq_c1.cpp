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
#include <WiFiClientSecure.h>

#include "Opaq_c1.h"
#include "Rf433ook.h"
#include "websites.h"

// non-class functions begin

static void ICACHE_FLASH_ATTR _deviceTaskLoop ( os_event_t* events )
{
  opaq_controller.run_task_ds3231();
  opaq_controller.setClockReady();
  
  opaq_controller.run_task_rf433ook();
}

static void ICACHE_FLASH_ATTR _10hzLoop ( os_event_t* events )
{
  opaq_controller.run_task_nrf24();
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
  server ( ESP8266WebServer ( 80 ) ),
  timming_events ( Ticker() ),
  rtc ( RtcDS3231() ),
  radio ( RF24 ( 16, 15 ) ),
  html ( AcHtml() ),
  storage ( AcStorage() ),
  str ( String() )
{
  str.reserve ( 2048 );
}


void OpenAq_Controller::setup_controller()
{
  delay ( 2000 );

  // initialize seed for code generation
  randomSeed ( ESP.getCycleCount() );

  // setup serial for a baundrate of 115200
  Serial.begin ( 115200 );
  Serial.setDebugOutput ( true );

  if ( storage.getSignature() != SIG )
  {
    factory_defaults ( SIG ); // uncomment to set the factory defaults
    Serial.println ( F("default settings applied.") );
  }

  // Initialize file system.
  if (!SPIFFS.begin())
  {
    Serial.println(F("Failed to mount file system"));
    ESP.restart();
  }

  // read openaq mode from eeprom
  bool mode = storage.getModeOperation() & 0b00000001;

  if ( mode )
  {
    const char* ssid = storage.getSSID();

    Serial.print ( F("SSID: ") );
    Serial.println ( ssid );
    
    WiFi.mode(WIFI_AP);
    
    // setup the access point
    WiFi.softAP ( ssid , storage.getPwd() ); // with password and fixed ssid
  }
  else
  {
    // or setup the station
    const char* ssid = storage.getClientSSID();
    const char* pwd = storage.getClientPwd();
    
    Serial.print ( F("Connecting to ") );
    Serial.println ( ssid );

    WiFi.begin ( ssid, pwd );

    int count_tries = 0;

    while ( WiFi.status() != WL_CONNECTED )
    {
      delay ( 500 );
      Serial.print ( "." );
      count_tries++;

      if (count_tries > 100)
      {
        Serial.println ( F("returning to AP mode. Reboot.") );
        storage.enableSoftAP();
        storage.save();
        ESP.restart();
      }
    }

    Serial.println ( F("WiFi connected") );
    Serial.println ( F("IP address: ") );
    Serial.println ( WiFi.localIP() );
  }

  // setup webserver
  server.on ( "/"      , std::bind ( &OpenAq_Controller::handleRoot, this ) );
  server.on ( "/light" , std::bind ( &OpenAq_Controller::handleLight, this ) );
  server.on ( "/clock" , std::bind ( &OpenAq_Controller::handleClock, this ) );
  server.on ( "/power" , std::bind ( &OpenAq_Controller::handlePower, this ) );
  server.on ( "/advset", std::bind ( &OpenAq_Controller::handleAdvset, this ) );
  server.on ( "/global", std::bind ( &OpenAq_Controller::handleGlobal, this ) );

  // setup webfiles for webserver
  for ( int fl = 0; fl < N_FILES; fl++ )
  {
    server.on ( files[fl].filename, [ = ]()
    {
      server.sendHeader ( "Content-Length", String ( files[fl].len ) );
      server.send_P ( 200, files[fl].mime, ( PGM_P )files[fl].content,
                      files[fl].len );
    } );
  }

  server.onNotFound ( [ = ]()
  {
    server.send ( 404, "  text/plain", String ( "    " ) );
  } );

  // start server
  server.begin();

  // RF433 setup
  Rf433_transmitter.set_pin ( 2 );
  Rf433_transmitter.set_encoding ( Rf433_transmitter.CHANON_DIO_DEVICE );

  // RTC setup
  rtc.Begin();
  Wire.begin ( 4, 5 );

  // NRF24 setup and radio configuration
  radio.begin();
  radio.setChannel(1);
  radio.setDataRate(RF24_1MBPS);
  radio.setAutoAck(false);
  //radio.disableCRC();
  radio.openWritingPipe( 0x5544332211LL ); // set address for outcoming messages
  radio.openReadingPipe( 1, 0x5544332211LL ); // set address for incoming messages
  
  // manual test
  //uint8_t buf[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
  //radio.write_register ( TX_ADDR, buf, 5 );
  //radio.write_register ( SETUP_AW, 0x3 );
  //radio.write_register ( EN_AA, 0x0 ); // mandatory - no ACK
  //radio.write_register ( EN_RXADDR, 0x0 );
  //radio.write_register ( SETUP_RETR, 0x0 );
  //radio.write_register ( RF_CH, 0x1 );
  //radio.write_register ( RF_SETUP, 0x7 );
  //radio.write_register ( CONFIG, 0xE ); // mandatory

  //radio.startListening();

  // for debug purposes of radio transceiver
  radio.printDetails();

  // registers deviceTask in the OS control structures
  system_os_task ( _deviceTaskLoop, deviceTaskPrio, deviceTaskQueue,
                   deviceTaskQueueLen );

  system_os_task ( _10hzLoop, _10hzLoopTaskPrio, _10hzLoopQueue,
                   _10hzLoopTaskQueueLen );

  // Attach deviceTask event trigger function for periodic releases
  timming_events.attach_ms ( 2000, _devicesTask_timmingEvent );

  t_evt.attach_ms ( 100, _10hzLoop_timmingEvent );
}

int count=0;
void OpenAq_Controller::run_controller()
{
  server.handleClient();

  /*if (count == 100000)
  {
    //opaq_controller.run_task_rf433ook();
    count=0;
  }
  count++;*/

}

void OpenAq_Controller::updatePowerOutlets ( uint8_t pdeviceId )
{
  bool vector[36] = { 0 };
  // vector is organized as follows :
  //              |---------------------------------------ID------------------------------------|--GROUPFLAG--|--ON/OFF--|---GROUPID---|-DIMMER-|
  // GLOBAL OFF -- unbinds everything
  // encoding   : 01 01 10 10 10 10 01 10 10 10 10 10 10 01 01 01 10 01 10 10 10 10 10 10 10 01 |       10         01    | 01 01 01 01 | UNDEF
  // bit stream : 0  0  1  1  1  1  0  1  1  1  1  1  1  0  0  0  1  0  1  1  1  1  1  1  1  0  |       1          0     |  0  0  0  0 | UNDEF
  // GLOBAL ON -- do not bind (binds are only allowed when groupflag is zero)
  // bit stream : 0  0  1  1  1  1  0  1  1  1  1  1  1  0  0  0  1  0  1  1  1  1  1  1  1  0  |       1          1     |  0  0  0  0 | UNDEF

  // get device code id
  uint32_t code = storage.getPDeviceCode ( pdeviceId );

  int j = 0;
  while ( j < 26 && code )
  {
    vector[j++] = code & 1;
    code >>= 1;
  }
  
  // get state from settings
  uint8_t state = storage.getPDeviceState ( pdeviceId );

  if ( state == OFF )
  {
    vector[26] = 0;
    vector[27] = 0;
  }
  else if ( state == UNBINDING )
  {
    vector[26] = 1;
    vector[27] = 0;
  }
  else if ( state == BINDING )
  {
    // set group flag to zero for allowing bind operations
    vector[26] = 0;
    vector[27] = 1;
  }
  else
  {
    vector[26] = 1;
    vector[27] = 1;
  }

  for ( int i = 0; i < 36; i++ )
  {
    DEBUGV ( "%d,", vector[i] );
  }
  DEBUGV ( "\n" );

  for ( int i = 0; i < 3; i++ )
  {
    Rf433_transmitter.sendMessage ( vector );
    delayMicroseconds ( 10000 );
  }
}

uint16_t normalizeClock(RtcDateTime * clock, const uint16_t a, const uint16_t b )
{
  float clktmp = ( ( float )clock->Hour() ) + ( ( ( float )clock->Minute() ) / 60 )
               + ( ( ( float )clock->Second() ) / 3600 );
               
  clktmp = (clktmp*255.)/24.;

  return (uint16_t)clktmp;
}

void OpenAq_Controller::run_task_rf433ook()
{
  // check if some device is binding...
  for ( int pdeviceId = 1; pdeviceId <= storage.getNumberOfPowerDevices();
        pdeviceId++ )
  {
    if ( storage.getPDeviceState ( pdeviceId ) == BINDING )
    {
      // do binding

      updatePowerOutlets ( pdeviceId );

      return;
    }
  }

  // check if some device is unbinding
  for ( int pdeviceId = 1; pdeviceId <= storage.getNumberOfPowerDevices();
        pdeviceId++ )
  {
    if ( storage.getPDeviceState ( pdeviceId ) == UNBINDING )
    {
      // do unbinding

      updatePowerOutlets ( pdeviceId );

      return;
    }
  }

  static bool wait_next_change[N_POWER_DEVICES] = { false };
  static int pdeviceId = 1;
  
  // normal execution
  // ----------------------------------
  DEBUGV ( "ac:: power %d\r\n", storage.getPDeviceState ( pdeviceId ) );

  pstate auto_state = (pstate)storage.getPDevicePoint( pdeviceId, normalizeClock(&clock, 0, 255) );

  // update Power outlet state according to the step functions
  DEBUGV("::ac nclock %d\n", normalizeClock(&clock, 0, 255));
  DEBUGV("::ac val %d\n", auto_state );
  
  // get state, if state is "permanent on"
  if( storage.getPDeviceState( pdeviceId ) == ON_PERMANENT )
  {
    // set on until next state change
    wait_next_change[ id2idx( pdeviceId ) ] = true;
    storage.setPowerDeviceState( pdeviceId, ON );
  }

  // get state, if state is "permanent off"
  if( storage.getPDeviceState( pdeviceId ) == OFF_PERMANENT )
  {
    // set off until next change
    wait_next_change[ id2idx( pdeviceId ) ] = true;
    storage.setPowerDeviceState( pdeviceId, OFF );
  }

  // detect change
  if ( storage.getPDeviceState( pdeviceId ) == AUTO )
  {
    wait_next_change[ id2idx( pdeviceId ) ] = false;
  }

  if ( !wait_next_change[ id2idx( pdeviceId ) ] )
  {
    if ( auto_state )
    {
      storage.setPowerDeviceState( pdeviceId, ON );
    }
    else
    {
      storage.setPowerDeviceState( pdeviceId, OFF );
    }
  }

  updatePowerOutlets ( pdeviceId );

  // re-send messages cyclically
  pdeviceId++;
  
  if (pdeviceId > storage.getNumberOfPowerDevices())
    pdeviceId = 1;
    
}

void OpenAq_Controller::run_task_ds3231()
{
  // update clock
  clock = rtc.GetDateTime();
}


/*--------------------  nrf24 device controller task  ---------------------*/

uint8_t calculate_lrc(uint8_t *buf)
{
  // calculate LRC - longitudinal redundancy check
  uint8_t LRC = 0;

  for ( int j = 1; j < 14; j++ )
  {
    LRC ^= buf[j];
  }

  return LRC;
}

void OpenAq_Controller::run_task_nrf24()
{
  static uint8_t deviceId=1;
  float clktmp;
  
  // read...

  /*radio.setChannel(100);
  
  uint8_t buf[32];

  //radio.stopListening();

  if ( radio.available() )
  {
    bool done = false;
    while (!done)
    {
      done = radio.read( buf, 32 );

      char tmp[5];
      for(int ib=0; ib < 32; ib++)
      {
        sprintf(tmp, "%02x ", buf[ib]);
        Serial.print(tmp);
      }
      Serial.println("recv.");
    }
  }*/
  
  //radio.stopListening();

  bool binding=false;
  
  // definition for Zetlight lancia dimmers
  // ---------------------------------------------------------------------------
  if ( storage.getDeviceType ( deviceId ) == ZETLIGHT_LANCIA_2CH )
  {
    // compute signal values according to the clock
    uint8_t signal1 = 0;
    uint8_t signal2 = 0;

    if ( !isClockReady() )
      return;

    clktmp = ( ( float )clock.Hour() ) + ( ( ( float )clock.Minute() ) / 60 )
             + ( ( ( float )clock.Second() ) / 3600 );
    storage.getLinearInterpolatedPoint ( deviceId, 1, clktmp, &signal1 );
    storage.getLinearInterpolatedPoint ( deviceId, 2, clktmp, &signal2 );

    // normal message
    uint8_t buf[32] = {
    //<3byte static signature>
      0xEE, 0xD, 0xA,
    // <------------------------------ LIGHT -------------------------------------------------->
    //  <Mode>= DAM(0x03); SUNRISE(0xb6); DAYTIME(0xb6); SUNSET(0xD4); NIGHTTIME(0x06)
    //  <Mode2>= DAM(0x00); SUNRISE(0x15); DAYTIME(0x58); SUNSET(0x2A); NIGHTIME(0x2A)
    // Normal mode (N_MODE)
    //<Mode1>     <value1>  <Mode2>    <value2>   <N_MODE>             < binding mode >  <groupId>    <LRC>
      0x00,      signal1,    0x00,     signal2,    0x10,     0x0, 0x0, 0x0,  0x0,  0x0,     0x0,      0x00,
    //<unknown> = 0xEC
      0xEC, 
    //<fixed values>
      0x0, 0x0, 0x0,
    //<5bytes controllerID>
      0xF2, 0x83, 0x1D, 0x4A, 0x20,
    //<fixed values>
      0x0, 0x0,
    //<unknown bytes>
      0x68, 0x71,
    //<2bytes group binding>
      0x00, 0x0,   // means nothing
    //<unknown values>
      0x0, 0x0};

    buf[14] = calculate_lrc(buf);
    

    // binding message
    uint8_t binding_data[32] = {
    //<3byte static signature>
      0xEE, 0x0D, 0x0A,
    // BINDING mode
    //                                            <N_MODE>             < binding mode >  <groupId>    <LRC>
      0x0,         0x0,      0x0,      0x0,        0x0,      0x0, 0x0, 0x23, 0xdc, 0x0,    0x01,      0x00,
    //<unknown> = 0xEC
      0xEC, 
    //<fixed values>
      0x0, 0x0, 0x0,
    //<5bytes controllerID>
      0xF2, 0x83, 0x1D, 0x4A, 0x20,
    //<fixed values>
      0x0, 0x0,
    //<unknown bytes>
      0x68, 0x71,
    //<2bytes group binding>
      0x00, 0x00,   // 0x08, 0x06 -  0x7b, 0x15,
    //<unknown values>
      0x0, 0x0};

    binding_data[14] = calculate_lrc(binding_data);

    uint8_t * codepointer = storage.getCodeId( deviceId );

    if( storage.getLState( deviceId ) == ON )
    {
      //DEBUGV("ac:: ON msg sent\n");

      buf[19] = codepointer[0];
      buf[20] = codepointer[1];
      buf[21] = codepointer[2];
      buf[22] = codepointer[3];
      buf[23] = codepointer[4];
      
      radio.startWrite ( buf, 32 );
      
    }

    if( storage.getLState( deviceId ) == BINDING && !binding )
    {
      binding = true;
      //DEBUGV("ac:: BIND msg sent\n");
      
      radio.setChannel(100);

      binding_data[19] = codepointer[0];
      binding_data[20] = codepointer[1];
      binding_data[21] = codepointer[2];
      binding_data[22] = codepointer[3];
      binding_data[23] = codepointer[4];
        
      binding_data[28] = 0x7B;
      binding_data[29] = 0x15;
      
      radio.startWrite ( binding_data, 32 );

      delayMicroseconds(10000);
      radio.setChannel(1);
        
    }

  }
  // Zetlight definition END

  //radio.startListening();

  deviceId++;
  
  if (deviceId > storage.getNumberOfLightDevices())
    deviceId = 1;
}

/*----------------  Set default settings for Opaq hardware  -----------------*/

void OpenAq_Controller::factory_defaults ( uint8_t sig )
{
  storage.defaults ( sig );
  storage.save();
}

/*======================  End of Opaq public methods  =======================*/




/*=============================================================================
=                      Opaq controller private handlers                       =
=============================================================================*/

void sendBlockS(String* str, ESP8266WebServer *sv)
{
  int index;
  for (index=0; index < floor((*str).length()/HTTP_DOWNLOAD_UNIT_SIZE); index++)
  {
    sv->client().write((*str).c_str() + (index * HTTP_DOWNLOAD_UNIT_SIZE), HTTP_DOWNLOAD_UNIT_SIZE);
  }
  
  if ( index != 0 )
  {
    *str = (char *)((*str).c_str() + (index * HTTP_DOWNLOAD_UNIT_SIZE));
  }

  DEBUGV( "ac:: B %d", index );
}

void sendBlockCS(String* str, ESP8266WebServer *sv, uint16_t* count)
{
  int index;
  for (index=0; index < floor((*str).length()/HTTP_DOWNLOAD_UNIT_SIZE); index++)
  {
    *count += HTTP_DOWNLOAD_UNIT_SIZE;
  }
  
  if ( index != 0 )
  {
    *str = (char *)((*str).c_str() + (index * HTTP_DOWNLOAD_UNIT_SIZE));
  }

  DEBUGV( "ac:: CS %d", *count );
}

std::function<void(String*)> OpenAq_Controller::sendBlockGlobal( ESP8266WebServer* sv, uint16_t* count, uint8_t* step )
{
    if ( *step )
    {
      return std::function<void(String*)>( [&sv]( String *str ) -> void { sendBlockS(str, sv); } );
    }
    else
    {
      return std::function<void(String*)>( [&sv,&count]( String *str ) -> void { sendBlockCS(str, sv, count); } );
    }
};

void OpenAq_Controller::handleRoot()
{
  ESP8266WebServer *sv = &server;
  uint16_t count = 0;
  uint8_t step;
  
  // sends webpage content in chunks

  // first calculate the expected size of the webpage and then sends the webpage
  for( step=0; step < 2; step++ )
  {
    // reset global str
    str = "";

    if( step == 1 )
    {
      server.sendHeader ( "Content-Length", String ( count ) );
      server.send ( 200, "text/html", String ( "" ) );
    }
    
    str.concat ( html.get_begin() );
    str.concat ( html.get_header() );
    str.concat ( html.get_body_begin() );
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

void zetlight_template1( uint8_t deviceId, AcStorage * const lstorage )
{
  auto apply_template = [&lstorage]( uint8_t deviceId, uint8_t signalId, uint8_t signal[SIGNAL_LENGTH][SIGNAL_STEP_SIZE], const uint8_t signal_len )
  {
    for (uint8_t i=1; i <= SIGNAL_LENGTH; i++)
    {
      lstorage->setPointXLD( id2idx(deviceId), id2idx(signalId), i, signal[i-1][0] );
      lstorage->setPointYLD( id2idx(deviceId), id2idx(signalId), i, signal[i-1][1] );
    }
    
    lstorage->setLDeviceSignalLength( id2idx(deviceId), id2idx(signalId), signal_len );
  };
  
  uint8_t signal1[SIGNAL_LENGTH][SIGNAL_STEP_SIZE] = { {28,0}, {63,32}, {24,79}, {0,112}, {235,157}, {242,192},  {106,233},  {28,255},  {0,0},  {0,0},  {0,0},  {0,0},  {0,0},  {0,0},  {0,0},  {0,0} }; // number of steps 8
  uint8_t signal2[SIGNAL_LENGTH][SIGNAL_STEP_SIZE] = { {0,0}, {0,109}, {234,155}, {244,187}, {147,206}, {0,255}, {0,0},  {0,0}, {0,0},  {0,0},  {0,0},  {0,0},  {0,0},  {0,0},  {0,0},  {0,0} }; // number of steps 6
  
  apply_template( deviceId, 1, signal1, 8 );
  apply_template( deviceId, 2, signal2, 6 );
}

void OpenAq_Controller::handleLight()
{
  // lets send light configuration
  
  if(server.hasArg("adevice"))
  {
    storage.addLightDevice();
    
    server.send(200, "text/html", String(F("<h3>Add device done</h3>")));
    
    return;
  }
  
  if(server.hasArg("sigdev") && server.hasArg("asigid") && server.hasArg("asigpt") && server.hasArg("asigxy") && server.hasArg("asigvalue"))
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

  /* update selected device */
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

  ESP8266WebServer *sv = &server;
  uint16_t count = 0;
  uint8_t step;

  // first calculate the expected size of the webpage and then sends the webpage
  for( step=0; step < 2; step++ )
  {
    // reset global str
    str = String("");

    if( step == 1 )
    {
      server.sendHeader ( "Content-Length", String ( count ) );
      server.send ( 200, "text/html", String ( "" ) );
    }
  
    str += html.get_begin();
    str += html.get_header_light();
    str += html.get_body_begin();
    str += html.get_menu();
    sendBlockGlobal(sv, &count, &step)(&str);
    
    html.get_light_script( &storage, &str, sendBlockGlobal( sv, &count, &step ) );
    str += html.get_light_settings_mbegin();
    sendBlockGlobal(sv, &count, &step)(&str);
    
    html.get_light_settings_mclock(&str, clock);
    sendBlockGlobal(sv, &count, &step)(&str);
    
    str += html.get_light_settings_mend();
    sendBlockGlobal(sv, &count, &step)(&str);
    
    RtcTemperature temp = rtc.GetTemperature();
    str += String(temp.AsWholeDegrees());
    str += ".";
    str += String(temp.GetFractional());
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

  ESP8266WebServer *sv = &server;
  uint16_t count = 0;
  uint8_t step;

  // first calculate the expected size of the webpage and then sends the webpage
  for( step=0; step < 2; step++ )
  {
    // reset global str
    str = "";

    if( step == 1 )
    {
      server.sendHeader ( "Content-Length", String ( count ) );
      server.send ( 200, "text/html", String ( "" ) );
    }
  
    str.concat( html.get_begin() );
    str.concat( html.get_header() );
    str.concat( html.get_body_begin() );
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

  rtc.SetDateTime(dt);
  
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
    ESP.restart();

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
}


void OpenAq_Controller::ota()
{
  WiFiClientSecure client;
  
  if ( !client.connect("http://raw.githubusercontent.com/anmaped/opaq/master/tools/loop_test.sh", 80) )
  {
    Serial.println("unable to connect");
    server.send(200, "text/html", String(F("<h3>Unable to connect.</h3>")));
  }

  uint8_t tmp[30 + 1] = "";
  tmp[30]='\0';
  while( client.available() )
  {
    client.read(tmp, 30);
    Serial.print((char *)tmp);
  }
  
}

/*=====  End of Opaq controller private methods  ======*/

OpenAq_Controller opaq_controller;

