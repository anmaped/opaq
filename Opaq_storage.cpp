/*
 *  Opaq is an Open AQuarium Controller firmware. It has been developed for
 *  supporting several aquarium devices such as ligh dimmers, power management
 *  outlets, water sensors, and peristaltic pumps. The main purpose is to
 *  control fresh and salt water aquariums.
 *
 *    Copyright (c) 2016 Andre Pedro. All rights reserved.
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
 
#include "Opaq_storage.h"
#include "Opaq_com.h"
#include "interpolate.h"

#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <HashMap.h>

#include <libtar.h>

#define DEBUGV_ST(...) ets_printf(__VA_ARGS__)

Opaq_storage storage = Opaq_storage();


Opaq_storage::Opaq_storage() :
  faqdim(Opaq_st_plugin_faqdim ()),
  pwdevice(Opaq_st_plugin_pwdevice()),
  wifisett(Opaq_st_plugin_wifisett()),
  touchsett(Opaq_st_plugin_touchsett())
{
  
}

void Opaq_storage::defaults ( uint8_t sig )
{
  // let's create the wifi configuration file
  // with: signature; ssid; pwd; clientssid; clientpwd; state; wifimode
  
  //SPIFFS.format();
  //SPIFFS.begin();

  // write signature
  writeSignature(sig);

  // [TODO]
  // set all settings for all plugins
  faqdim.defaults();
  pwdevice.defaults();
  wifisett.defaults();
  touchsett.defaults();

  
  /**signature = sig;

  // SSID setting
  //static const char* lssid PROGMEM = "opaq-0001";
  char tmp[20];
  sprintf(tmp, "opaq-%08x", ESP.getChipId());
  memcpy ( ssid, tmp, strlen(tmp) );

  // default PWD
  //static const char* lpwd PROGMEM = "opaqopaq";
  //memcpy_P ( pwd, lpwd, strlen_P(lpwd) );
  pwd = "opaqopaq";

  // operation modes for controller
  // access point mode enabled - bit 0b---- ---X
  // *op = 0b00000001;

  // enable softAP by default
  enableSoftAP();
  
  *numberOfLightDevices = 0;
  *numberOfPowerDevices = 0;*/

 
  ESP.restart();
  
}

void Opaq_storage::initOpaqC1Service()
{
  /*
  ESP8266httpClient client;

  auto storer = [](WiFiClient tcp, size_t l, const char * name)
  {
    uint8_t buffer[WIFICLIENT_MAX_PACKET_SIZE];
    size_t remaining_size=l, buffer_size;

    // open file
    File f = SPIFFS.open(name, "w");
    
    while (remaining_size > 0)
    {
      if (remaining_size - WIFICLIENT_MAX_PACKET_SIZE > 0) {
        buffer_size = tcp.read(buffer, WIFICLIENT_MAX_PACKET_SIZE);
      }
      else
      {
         buffer_size = tcp.read(buffer, remaining_size);
      }
      // save to file buffer with len buffer_size
      f.write( buffer, buffer_size );

      remaining_size -= buffer_size;

      Serial.println("#NEXT...");

      yield();
    }

    f.close();

    Serial.println("File transferred.");
  };
  
  // get default opaq c1 UI
  // request jquery file
  client.open("http://ec2-52-29-83-128.eu-central-1.compute.amazonaws.com/");
  client.requestFile("http://ec2-52-29-83-128.eu-central-1.compute.amazonaws.com/opaq/c1/page/jquery.mobile-1.4.5.min.js", storer );
  
  // request mobile jquery file
  
  
  // get default opaq c1 settings
  */

auto download_file = [](const char * filename)
{
  HTTPClient http;

  File f = SPIFFS.open((String("/www/") + filename).c_str(),"w");

  if (!f)
  {
    return;
  }
  
  Serial.print("[HTTP] begin...\n");
  Serial.println(filename);

  // configure server and url
  http.begin((String("http://ec2-52-29-83-128.eu-central-1.compute.amazonaws.com/opaq/c1/www/") + filename).c_str() );

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();
  
  if(httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if(httpCode == HTTP_CODE_OK) {

      // get lenght of document (is -1 when Server sends no Content-Length header)
      int len = http.getSize();

      // create buffer for read
      uint8_t buff[1536] = { 0 };

      // get tcp stream
      WiFiClient * stream = http.getStreamPtr();

      // read all data from server
      while(http.connected() && (len > 0 || len == -1)) {
        // get available data size
        size_t size = stream->available();

        if(size) {
          // read up to 1536 byte
          int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

          // write it to Serial
          //Serial.write(buff, c);
          f.write(buff, c);

          if(len > 0) {
            len -= c;
          }
        }
        
        yield();
      }

      Serial.println();
      Serial.print("[HTTP] connection closed or file end.\n");

      f.close();
    }
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
};



  /*download_file("oaq_sett_fadimmer.html");
  download_file("oaq_sett_pdev.html");
  download_file("opaqc1.html");
  download_file("opaqc1_index.html");
  download_file("opaqc1_settings.html");*/

  
  //libtar_list("/www/www.tar");

  /*SPIFFS.end();

  SPIFFS.format();

  SPIFFS.begin();
*/

  //SPIFFS.format();

  // check the downloaded file
  //download_file("www.tar");


  libtar_extract("/www/www.tar", "/www");

  SPIFFS.remove("/www/www.tar");


}

const uint8_t Opaq_storage::getSignature()
{
  byte sig;

  File fl = SPIFFS.open("/sett/sig", "r");

  if(fl)
  {
    sig = fl.read();
    fl.close();
  }
  
  return sig;
}

void Opaq_storage::writeSignature(byte sig)
{
  File fl = SPIFFS.open("/sett/sig", "w");

  if(fl)
  {
    fl.write(sig);
    fl.close();
  }
}






File& FileIterator::operator* ()
{
  last_file = _p_vec->dirlist().openFile("r");
  return last_file;
}

const FileIterator& FileIterator::operator++ ()
{
  last_file.close();
  
  if(!_p_vec->dirlist().next())
  {
    go_on = false;
  }

  return *this;
}







void Opaq_st_plugin::load(File& fl, String& toParse)
{
  // json to mem  
  uint8_t buf[64];
  int len, global_len;

  global_len = fl.size();

  while ((len = fl.read(buf, 64)) != 0 )
  {
    buf[len] = '\0';
    toParse += (char*)buf;
  }
  
  Serial.println(toParse);
}

bool Opaq_st_plugin::load(const char * filename, String& toParse)
{
  if ( !SPIFFS.exists(filename) )
    return true;

  Serial.println("Loading...");
  
  File fl = SPIFFS.open(filename, "r+");

  load(fl, toParse);
  
  fl.close();

  return false;
}






void Opaq_st_plugin_wifisett::parseConfiguration(const char * param, String& out)
{
  String toParse;
  StaticJsonBuffer<512> jsonBuffer;

  Serial.println(param);
  
  if ( !load("/sett/wifi/conf.json", toParse) )
  {
    Serial.println("TEST!");
    JsonObject& root = jsonBuffer.parseObject(toParse);

    out = (const char *)root[param];

    Serial.println(out);
  }
}

void Opaq_st_plugin_wifisett::changeConfiguration(const char * param, const char * value)
{
  String toParse, tmp = F("/sett/wifi/conf.json");
  StaticJsonBuffer<512> jsonBuffer;
  
  JsonObject& root = (load(tmp.c_str(), toParse))? jsonBuffer.createObject() : jsonBuffer.parseObject(toParse);
  
  root[param] = value;

  // save file
  PrintFile file = PrintFile(tmp.c_str(), "w");

  root.printTo(file);
}

void Opaq_st_plugin_wifisett::defaults()
{
  char tmp[30];
  sprintf(tmp, "opaq-%08x", ESP.getChipId());
  
  setSSID(tmp);
  setPwd("opaqopaq");

  enableSoftAP();
  
}

void Opaq_st_plugin_wifisett::getSSID(String& ssid)
{
  parseConfiguration("wssid", ssid);
}

void Opaq_st_plugin_wifisett::getPwd(String& pwd)
{
  parseConfiguration("wpwd", pwd);
}

void Opaq_st_plugin_wifisett::getClientSSID(String& ssid)
{
  parseConfiguration("wclientssid", ssid);
}

void Opaq_st_plugin_wifisett::getClientPwd(String& pwd)
{
  parseConfiguration("wclientpwd", pwd);
}

void Opaq_st_plugin_wifisett::getMode(String& mode)
{
  parseConfiguration("wmode", mode);
}

void Opaq_st_plugin_wifisett::setSSID( String ssid )
{
  changeConfiguration("wssid", ssid.c_str());
}

void Opaq_st_plugin_wifisett::setPwd( String pwd )
{
  changeConfiguration("wpwd", pwd.c_str());
}

void Opaq_st_plugin_wifisett::setClientSSID( String ssid )
{
  changeConfiguration("wclientssid", ssid.c_str());
}

void Opaq_st_plugin_wifisett::setClientPwd( String pwd )
{
  changeConfiguration("wclientpwd", pwd.c_str());
}

uint8_t Opaq_st_plugin_wifisett::getModeOperation()
{
  String tmp;
  parseConfiguration("wmode", tmp);
  
  return tmp == "softAP";
}

void Opaq_st_plugin_wifisett::enableSoftAP()
{
  changeConfiguration("wmode", "softAP"); 
}

void Opaq_st_plugin_wifisett::enableClient()
{
  changeConfiguration("wmode", "client");
}






void Opaq_st_plugin_faqdim::defaults()
{
  
}

void Opaq_st_plugin_faqdim::add()
{
  DynamicJsonBuffer jsonBuffer;
  char code[5*2+1];
  String filename;

  // try to find a valid id
  do
  {
     // let's generate the id
    for(byte i=0; i< 10; i+=2)
    {
      sprintf(&code[i], "%02X", random ( 0x0, 0xff ));
    }
    code[5*2] = '\0';

    getFilename(filename, code);
  }
  while(SPIFFS.exists(filename.c_str()));
  
  // lets create the empty file
  File tmp = SPIFFS.open(filename.c_str(), "w");
  tmp.close();

  // printer for json
  PrintFile file = PrintFile(filename.c_str());
  
  // light device properties
  // (codeID, type, description, state)
  JsonObject& root = jsonBuffer.createObject();
  root["adimid"] = code;
  root["type"] = (unsigned int)OPENAQV1;
  root["description"] = code;
  root["state"] = "on";
  root["cursor"] = 4; // the size of the default data

  // [['00:00',1],['01:00',1] ...]
  JsonArray& data = root.createNestedArray("data");

  // number of channels
  for(byte j=0; j<4; j++)
  {
     JsonArray& subdata = data.createNestedArray();

    for(byte i=0; i < 24; i++)
    {
      JsonArray& subsubdata = subdata.createNestedArray();
      
      subsubdata.add(i*60*60*1000);
      subsubdata.add(j*10+i);
    }
  
  }

  root["size"] = jsonBuffer.size();
  
  root.printTo(file);
}

void Opaq_st_plugin_faqdim::save(const char * code, const uint8_t * content, size_t len)
{ 
  String filename;
  
  getFilename(filename, code);

  Serial.println("FILE SAVE:");
  Serial.println(filename);
  
  // file to save
  File tmp = SPIFFS.open(filename, "w");

  tmp.write(content, len);
  
  tmp.close();
}

void Opaq_st_plugin_faqdim::getDir(String& filename)
{
  filename = F("/sett/adim/");
}

void Opaq_st_plugin_faqdim::getFilename(String& filename, const char * code)
{
  getDir(filename);
  filename += code;
  filename += F(".json");
}

void Opaq_st_plugin_faqdim::remove(const char* code)
{
  String filename = "";
  
  getFilename(filename, code);
  
  // for each file in /sett/adim directory do
  Dir directory = SPIFFS.openDir("/sett/adim");
  
  while ( directory.next() )
  {
    if( directory.fileName() == filename )
    {
      Serial.println("ADIM FILE REMOVED");
      SPIFFS.remove(directory.fileName());
    }
  }
}

void Opaq_st_plugin_faqdim::run()
{
  DynamicJsonBuffer jsonBuffer;
  String tmp;

  // get clock
  RtcDateTime date;
  communicate.getClock(date);
  unsigned long clock_value = (date.Second() + date.Minute()*60 + date.Hour()*60*60)*1000;

  auto getState = [](JsonObject& obj, unsigned long clk, LinkedList<byte>& signal)
  {
    // get array
    JsonArray& arr = obj["data"];
    
    // create linked list for each elements
    for(int i=0; arr[i].is<JsonArray&>(); i++)
    {
      LinkedList<std::pair<unsigned long, byte>> signallist;

      JsonArray& subarr = arr[i];

      for (int j=0; subarr[j].is<JsonArray&>(); j++)
      {
        JsonArray& subsubarr = subarr[j];

        unsigned long time_ms = subsubarr[0];
        byte value = subsubarr[1];
      
        DEBUGV_ST ( "ac:: adim %d, %d %d\r\n", i, time_ms, value);
      
        signallist.add( std::make_pair(time_ms, value) );

      }

      byte new_value = hermiteInterpolate<unsigned long, byte>(signallist, clk, 0, 0);
      
      DEBUGV_ST ( "ac:: adim %d\r\n", new_value);

      signal.add(new_value);

    }

  };


  // for each aquarium dimmer device
  Serial.println("ST!");
  // probably is not safe at all (files can be removed... concurrent accesses)
  Directory dr = Directory("/sett/adim");
  for ( File fl : dr )
  {
    if(fl == NULL)
      continue;
    
    Serial.println(fl.size());

    // get fl to memory to parse it
    tmp = ""; // [TODO] put file to buffer
    
    load(fl, tmp);
    
    JsonObject& adimfile = jsonBuffer.parseObject(tmp);

    if( !adimfile.success() )
      break;

    // convert code to integer
    unsigned long code = strtol((const char *)adimfile["adimid"], NULL, 16);
    String state       = String((const char *)adimfile["state"]);
    String type        = String((const char *)adimfile["type"]); // [TODO] restrict that by type

    DEBUGV_ST ( "ac:: adim %d %s %lu\r\n", code, state.c_str(), clock_value);

    // store computed signal values according to the clock value
    LinkedList<byte> signalstate_list;

    // for zetlancia case
    if( type == "zetlancia")
    {
      tmp_type = ZETLIGHT_LANCIA_2CH;
    }
    

    // check if some device is binding...
    if ( state == "bind" )
    {
      send(code, NRF24_BINDING);
    }

    if ( state == "unbind" )
    {
      send(code, NRF24_UNBINDING);
    }

    if ( state == "listen" )
    {
      send(code, NRF24_LISTENING);
    }

    if ( state == "auto" )
    { 
      getState(adimfile, clock_value, signalstate_list);
      send(code, signalstate_list);
    }
    else if ( state == "on" )
    {
      send(code, NRF24_ON);
    }
    else if ( state == "off" )
    {
      send(code, NRF24_OFF);
    }

    optimistic_yield(10000);
  }

}

void Opaq_st_plugin_faqdim::send(unsigned int code, nrf24state state)
{
  RF24& radio = communicate.nrf24.getRF24();

  auto calculate_lrc = [](uint8_t *buf)
  {
    // calculate LRC - longitudinal redundancy check
    uint8_t LRC = 0;

    for ( int j = 1; j < 14; j++ )
    {
      LRC ^= buf[j];
    }

    return LRC;
  };

  if (tmp_type == ZETLIGHT_LANCIA_2CH)
  {
    uint8_t * codepointer = (uint8_t *) &code;

    communicate.spinlock();

    if ( state == NRF24_BINDING )
    {
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
        0x0, 0x0
      };

      binding_data[14] = calculate_lrc(binding_data);


      radio.stopListening();
      //DEBUGV("ac:: BIND msg sent\n");

      radio.setChannel(100);
      delayMicroseconds(10000);

      memcpy(&binding_data[19], &codepointer[0], 4);

      binding_data[28] = 0x7B;
      binding_data[29] = 0x15;

      radio.startWrite ( binding_data, 32 );

      delayMicroseconds(10000);
      radio.setChannel(1);

    }

    if ( state == NRF24_LISTENING )
    {
      // read...

      //radio.setChannel(100);
      uint8_t buf[32];
      bool a = true;
      String x = String("{\"nrf24M\" :[");
      
      if ( radio.available() )
      {
        bool done = false;
        //while (!done)
        {
          done = radio.read( buf, 32 );

          char tmp[10];
          
          for (int ib = 0; ib < 32; ib++)
          {
            sprintf(tmp, "\"%02x\"%c ", buf[ib], ib < 31 ? ',' : ' ' );
            x += tmp;
          }
          x += "]}";
        }
        a=false;
      }

      radio.startListening();

      if(a == false)
      {
        a=true;
        Serial.println(x);
        //[TODO] put that to output ... webSocket.sendTXT(0, x.c_str(), x.length());
      }
    }

    communicate.unlock();
  }


}

void Opaq_st_plugin_faqdim::send(unsigned int code, LinkedList<byte>& state)
{
  RF24& radio = communicate.nrf24.getRF24();

  auto calculate_lrc = [](uint8_t *buf)
  {
    // calculate LRC - longitudinal redundancy check
    uint8_t LRC = 0;

    for ( int j = 1; j < 14; j++ )
    {
      LRC ^= buf[j];
    }

    return LRC;
  };


  if (tmp_type == ZETLIGHT_LANCIA_2CH)
  {
    communicate.spinlock();


    uint8_t * codepointer = (uint8_t *) &code;

    if(state.size() < 2)
    {
      return;
    }

    float tmp = ((float)state.get(0))*(255/100);
    uint8_t signal1 = tmp;
    tmp = ((float)state.get(1))*(255/100);
    uint8_t signal2 = tmp;



    Serial.println("SIGNAL");
    Serial.println(signal1);
    Serial.println(signal2);

/*{"nrf24M" :["ee", "0d", "0a", "00", "91", "00", "9d", "10", "00", "00", "00", "00", "00", "00", "1b", "ec", "00", "00", "00",
  "e2", "0b", "f5", "37", "c0",
  "00", "00", "68", "71", "00", "00", "00", "00"  ]}*/


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
      0x0, 0x0
    };

    buf[14] = calculate_lrc(buf);


    radio.stopListening();
    //DEBUGV("ac:: ON msg sent\n");

    memcpy(&buf[19], &codepointer[0], 4);

    radio.startWrite ( buf, 32 );

    communicate.unlock();
  }

}








void Opaq_st_plugin_pwdevice::defaults()
{
  
}
void Opaq_st_plugin_pwdevice::add()
{
  DynamicJsonBuffer jsonBuffer;
  char code[7+1];
  String filename;

  const int nchan = 1;

  // try to find a valid id
  do
  {
    // let's generate the id
    sprintf(&code[0], "%07X", random ( 0x1, 0x3ffffff ));
    code[7] = '\0';

    getFilename(filename, code);
  }
  while(SPIFFS.exists(filename.c_str()));
  
  // lets create the empty file
  File tmp = SPIFFS.open(filename.c_str(), "w");
  tmp.close();

  // printer for json
  PrintFile file = PrintFile(filename.c_str());
  
  // power device properties
  // (codeID, type, description, state)
  JsonObject& root = jsonBuffer.createObject();
  root["pdevid"] = code;
  root["type"] = (unsigned int)CHACON_DIO;
  root["description"] = code;
  root["state"] = "auto";
  root["cursor"] = nchan; // the size of the default data

  // [['00:00',1],['01:00',1] ...]
  JsonArray& data = root.createNestedArray("data");

  // number of channels
  for(byte j=0; j<nchan; j++)
  {
     JsonArray& subdata = data.createNestedArray();

    bool inv = false;
    for(byte i=0; i < 24; i++)
    {
      JsonArray& subsubdata = subdata.createNestedArray();
      
      subsubdata.add(i*60*60*1000);
      subsubdata.add(inv);
      inv = !inv;
    }
  
  }

  root["size"] = jsonBuffer.size();
  
  root.printTo(file);

}

void Opaq_st_plugin_pwdevice::save(const char * code, const uint8_t * content, size_t len)
{
  String filename;
  
  getFilename(filename, code);

  Serial.println("FILE SAVE:");
  Serial.println(filename);
  
  // file to save
  File tmp = SPIFFS.open(filename, "w");

  tmp.write(content, len);
  
  tmp.close();
}

void Opaq_st_plugin_pwdevice::getDir(String& filename)
{
  filename = F("/sett/pdev/");
}

void Opaq_st_plugin_pwdevice::getFilename(String& filename, const char * code)
{
  getDir(filename);
  filename += code;
  filename += F(".json");
}

void Opaq_st_plugin_pwdevice::remove(const char* code)
{
  String filename = "";
  
  getFilename(filename, code);
  
  // for each file in /sett/adim directory do
  Dir directory = SPIFFS.openDir("/sett/pdev");
  
  while ( directory.next() )
  {
    if( directory.fileName() == filename )
    {
      Serial.println("PDEV FILE REMOVED");
      SPIFFS.remove(directory.fileName());
    }
  }
}

void Opaq_st_plugin_pwdevice::run()
{
  DynamicJsonBuffer jsonBuffer;
  String tmp;

  auto getState = [](JsonObject& obj, unsigned long clk)
  {
    // get array
    JsonArray& arr = obj["data"];
    
    // create linked list for each elements
    for(int i=0; arr[i].is<JsonArray&>(); i++)
    {
      LinkedList<std::pair<unsigned long, byte>> signallist;

      JsonArray& subarr = arr[i];

      for (int j=0; subarr[j].is<JsonArray&>(); j++)
      {
        JsonArray& subsubarr = subarr[j];

        unsigned long time_ms = subsubarr[0];
        byte value = subsubarr[1];
      
        DEBUGV_ST ( "ac:: pdev %d, %d %d\r\n", i, time_ms, value);
      
        signallist.add( std::make_pair(time_ms, value) );

      }

      byte new_value = hermiteInterpolate<unsigned long, byte>(signallist, clk, 0, 0);
      
      DEBUGV_ST ( "ac:: pdev %d\r\n", new_value);

    }

    

    return RF433_ON;
  };


  // get clock
  RtcDateTime date;
  communicate.getClock(date);
  unsigned long clock_value = (date.Second() + date.Minute()*60 + date.Hour()*60*60)*1000;

  
  Serial.println("ST!");
  // probably is not safe at all (files can be removed... concurrent accesses)
  Directory dr = Directory("/sett/pdev");
  for ( File fl : dr )
  {
    if(fl == NULL)
      continue;
    
    Serial.println(fl.size());

    // get fl to memory to parse it
    tmp = ""; // [TODO] put file to buffer
    
    load(fl, tmp);
    
    JsonObject& pwdevfile = jsonBuffer.parseObject(tmp);

    if( !pwdevfile.success() )
      break;

    // convert code to integer
    long code = strtol((const char *)pwdevfile["pdevid"], NULL, 16);
    String state =  String((const char *)pwdevfile["state"]);
    String type = String((const char *)pwdevfile["type"]); // [TODO] restrict that by type

    DEBUGV_ST ( "ac:: pdev %d %s %lu\r\n", code, state.c_str(), clock_value);

    // for chacon dio case
    if( type != "chacondio")
      break;
      
    // check if some device is binding...
    if ( state == "bind" )
    {
      send(code, RF433_BINDING);
    }

    if ( state == "unbind" )
    {
      send(code, RF433_UNBINDING);
    }

    if ( state == "auto" )
    { 
      send(code, getState(pwdevfile, clock_value));
    }
    else if ( state == "on" )
    {
      send(code, RF433_ON);
    }
    else if ( state == "off" )
    {
      send(code, RF433_OFF);  
    }

    optimistic_yield(10000);
  }
  
}

void Opaq_st_plugin_pwdevice::send(unsigned int code, short unsigned int state)
{
  static CreateHashMap(statemap, unsigned int, byte, 50);

  byte tmp[10];
  byte idx = 0;
  

  if ( statemap.contains(code) && state == statemap[code] && state != RF433_BINDING && state != RF433_UNBINDING)
  {
    return;
  }

  statemap[code] = state;
  
  
  // comunicate with coprocessor
  Serial.println(F("ASK AVR TO RECEIVE RF433 STREAM"));

  if( communicate.connect() )
    return;

  tmp[idx++] = SPI.transfer (ID_RF433_STREAM);
  delayMicroseconds (30);

  // payload length
  tmp[idx++] = SPI.transfer ( 0x05 );
  delayMicroseconds (30);
  
  tmp[idx++] = SPI.transfer ( (byte)code );
  delayMicroseconds (30);
  tmp[idx++] = SPI.transfer ( (byte)(code >> 8) );
  delayMicroseconds (30);
  tmp[idx++] = SPI.transfer ( (byte)(code >> 16) );
  delayMicroseconds (30);
  tmp[idx++] = SPI.transfer ( (byte)(code >> 24) );
  delayMicroseconds (30);
  tmp[idx++] = SPI.transfer ( state );
  delayMicroseconds (30);

  // dummy
  tmp[idx++] = SPI.transfer ( 0x10 );
  delayMicroseconds (30);
  
  communicate.disconnect();

  for(byte i=0; i < idx; i++)
    Serial.println("r: " + String(tmp[i]));

  Serial.println(F("AVR ASKED!"));
}






void Opaq_st_plugin_touchsett::defaults()
{
  
}

bool Opaq_st_plugin_touchsett::isTouchMatrixAvailable()
{
  return SPIFFS.exists("/sett/touch.json");
}

CAL_MATRIX& Opaq_st_plugin_touchsett::getTouchMatrixRef()
{ 
  return touch_cal_matrix_ref;
}

CAL_MATRIX Opaq_st_plugin_touchsett::getTouchMatrix()
{
  StaticJsonBuffer<400> jsonBuffer;
  String toParse;
  uint8_t buf[64];
  int len, global_len;
  
  // refresh settings from the file system
  File touch_settings_file = SPIFFS.open("/sett/touch.json", "r+");
  //touch_settings_file.read();
  touch_settings_file.seek (0, SeekEnd );
  global_len = touch_settings_file.position();
  touch_settings_file.seek (0, SeekSet );

  // test if there is free memory available
  // [TODO]

  while ((len = touch_settings_file.read(buf, 64)) != 0 )
  {
    buf[len] = '\0';
    toParse += (char*)buf;
  }
  
  touch_settings_file.close();

  Serial.println(toParse);

  JsonObject& root = jsonBuffer.parseObject(toParse);

  touch_cal_matrix.a = root["a"];
  touch_cal_matrix.b = root["b"];
  touch_cal_matrix.c = root["c"];
  touch_cal_matrix.d = root["d"];
  touch_cal_matrix.e = root["e"];
  touch_cal_matrix.f = root["f"];
  touch_cal_matrix.div = root["div"];
  touch_cal_matrix.endpoints[MIN_ENDPOINT].x = root["mix"];
  touch_cal_matrix.endpoints[MAX_ENDPOINT].x = root["max"];
  touch_cal_matrix.endpoints[MIN_ENDPOINT].y = root["miy"];
  touch_cal_matrix.endpoints[MAX_ENDPOINT].y = root["may"];
  
  return touch_cal_matrix;
}

void Opaq_st_plugin_touchsett::commitTouchSettings()
{
  StaticJsonBuffer<200> jsonBuffer;
  PrintFile file = PrintFile("/sett/touch.json");

  JsonObject& root = jsonBuffer.createObject();
  root["a"] = touch_cal_matrix.a;
  root["b"] = touch_cal_matrix.b;
  root["c"] = touch_cal_matrix.c;
  root["d"] = touch_cal_matrix.d;
  root["e"] = touch_cal_matrix.e;
  root["f"] = touch_cal_matrix.f;
  root["div"] = touch_cal_matrix.div;
  root["mix"] = touch_cal_matrix.endpoints[MIN_ENDPOINT].x;
  root["max"] = touch_cal_matrix.endpoints[MAX_ENDPOINT].x;
  root["miy"] = touch_cal_matrix.endpoints[MIN_ENDPOINT].y;
  root["may"] = touch_cal_matrix.endpoints[MAX_ENDPOINT].y;

  root.printTo(file);
  
}

