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

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

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


  HTTPClient http;

  File f = SPIFFS.open("/tmp/jquery.mobile-1.4.5.min.js","w");

  if (f == NULL)
  {
    return;
  }
  
  Serial.print("[HTTP] begin...\n");

  // configure server and url
  http.begin("http://ec2-52-29-83-128.eu-central-1.compute.amazonaws.com/opaq/c1/page/jquery.mobile-1.4.5.min.js");

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

}

const uint8_t Opaq_storage::getSignature()
{
  byte sig;

  File fl = SPIFFS.open("/sett/sig", "r");

  if( fl != NULL )
  {
    sig = fl.read();
    fl.close();
  }
  
  return sig;
}

void Opaq_storage::writeSignature(byte sig)
{
  File fl = SPIFFS.open("/sett/sig", "w");

  if( fl != NULL )
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
  static char currentId[32] = "";
  String tmp;

  auto getState = [](JsonObject& obj)
  {
    // get array
    JsonArray& arr = obj["data"];

    int time_ms, value;
    
    for(int i=0; arr[i].is < JsonArray&>(); i++)
    {
      JsonArray& subarr = arr[i];

      time_ms = subarr[0];
      value = subarr[1];

      
      DEBUGV_ST ( "ac:: pdev %d, %d %d\r\n", i, time_ms, value);

      // [TODO]
    }

    return RF433_ON;
  };

  // test if rf433 device is ready
  if ( !communicate.rf433.ready() )
    return;
  
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

    DEBUGV_ST ( "ac:: pdev %d %s\r\n", code, state.c_str());
      
    // check if some device is binding...
    if ( state == "bind" )
    {
      communicate.rf433.send(code, RF433_BINDING);
    }

    if ( state == "unbind" )
    {
      communicate.rf433.send(code, RF433_UNBINDING);
    }

    if ( state == "auto" )
    { 
      communicate.rf433.send(code, getState(pwdevfile));
    }
    else if ( state == "on" )
    {
      communicate.rf433.send(code, RF433_ON);
    }
    else if ( state == "off" )
    {
      communicate.rf433.send(code, RF433_OFF);  
    }

    optimistic_yield(10000);
  }


/*
  //static bool wait_next_change[N_POWER_DEVICES] = { false };
  static int pdeviceId = 1;

  // normal execution
  // ----------------------------------
  DEBUGV ( "ac:: power %d\r\n", storage.getPDeviceState ( pdeviceId ) );

  pstate auto_state = (pstate)storage.getPDevicePoint( pdeviceId, normalizeClock(&clock, 0, 255) );

  // update Power outlet state according to the step functions
  DEBUGV("::ac nclock %d\n", normalizeClock(&clock, 0, 255));
  DEBUGV("::ac val %d\n", auto_state );t

  // get state, if state is "permanent on"
  if ( storage.getPDeviceState( pdeviceId ) == ON_PERMANENT )
  {
    // set on until next state change
    wait_next_change[ id2idx( pdeviceId ) ] = true;
    storage.setPowerDeviceState( pdeviceId, ON );
  }

  // get state, if state is "permanent off"
  if ( storage.getPDeviceState( pdeviceId ) == OFF_PERMANENT )
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
  */


  
}




/*
void Opaq_storage::addSignal ( uint8_t deviceId, uint8_t signalId, uint8_t pointId,
                            uint8_t xy, uint8_t value )
{
  // make checks [TODO]
  if ( deviceId > 0 && signalId > 0 && pointId > 0 && value <= 255 )
  {
    if ( xy == 0 )
    {
      lightDevice[deviceId - 1].signal[signalId - 1][pointId * 2 - 2] = value;
    }
    else
    {
      lightDevice[deviceId - 1].signal[signalId - 1][pointId * 2 - 1] = value;
    }
  }
}

void Opaq_storage::getLinearInterpolatedPoint ( uint8_t deviceId, uint8_t signalId,
    float x, uint8_t* y )
{
  uint8_t x0 = 25, y0 = 25, x1 = 25, y1 = 25;
  bool finded = false;

  x = ( x / 24 * 255 );

  // find two points where value is between them
  if ( deviceId > 0 && signalId > 0 )
  {
    if ( lightDevice[deviceId - 1].signal[signalId - 1][S_LEN_EACH - 1] > 0 )
    {
      for ( int i = 0;
            i < lightDevice[deviceId - 1].signal[signalId - 1][S_LEN_EACH - 1] - 1; i++ )
      {
        if ( lightDevice[deviceId - 1].signal[signalId - 1][i * 2 + 1] <= x &&
             x < lightDevice[deviceId - 1].signal[signalId - 1][ ( i + 1 ) * 2 + 1] )
        {
          y0 = lightDevice[deviceId - 1].signal[signalId - 1][i * 2];
          x0 = lightDevice[deviceId - 1].signal[signalId - 1][i * 2 + 1];
          y1 = lightDevice[deviceId - 1].signal[signalId - 1][ ( i + 1 ) * 2];
          x1 = lightDevice[deviceId - 1].signal[signalId - 1][ ( i + 1 ) * 2 + 1];
          finded = true;
          break;
        }
      }

      if ( finded )
      {
        *y = y0 + ( ( uint8_t ) ( ( ( float ) ( y1 - y0 ) ) * ( ( ( float ) (
                                    x - x0 ) ) / ( ( float ) ( x1 - x0 ) ) ) ) );
        DEBUGV ( "::li %d %d %d %d %d %d \r\n", x0, y0, x1, y1, ( unsigned int )x, *y );
      }
      else
      {
        DEBUGV ( "::li not found\r\n" );
        *y = 26;
      }
    }
    else
    {
      *y = 25;
    }
  }
}

void Opaq_storage::setDeviceType ( const uint8_t deviceId, const uint8_t type )
{
  if ( deviceId > 0 )
  {
    lightDevice[deviceId - 1].type = type;
  }
}

uint8_t Opaq_storage::getDeviceType ( uint8_t deviceId )
{
  if ( deviceId > 0 && deviceId <= N_LIGHT_DEVICES )
    return lightDevice[deviceId - 1].type;

  return 0;
}

void Opaq_storage::addPowerDevice()
{
  // defensive case
  if ( *numberOfPowerDevices >= N_POWER_DEVICES )
    return;

  powerDevice[*numberOfPowerDevices].id = *numberOfPowerDevices + 1;
  powerDevice[*numberOfPowerDevices].type = CHACON_DIO;
  powerDevice[*numberOfPowerDevices].state = OFF;
  powerDevice[*numberOfPowerDevices].code = random ( 0x1, 0x3ffffff );

  char buf[FILE_DESC_SIZE + 1];
  sprintf( buf, "Description.%d", *numberOfPowerDevices );
  
  File psocketsFile = SPIFFS.open("/settings/psockets.json", "r+");

  if ( psocketsFile == NULL )
  {
    // create file
    psocketsFile = SPIFFS.open("/settings/psockets.json", "w");
  }
  
  psocketsFile.seek( FILE_DESC_SIZE * *numberOfPowerDevices, SeekSet ) ;
  psocketsFile.write( (uint8_t*)buf, FILE_DESC_SIZE );
  psocketsFile.close();

  ( *numberOfPowerDevices )++;
}

void Opaq_storage::setPowerDeviceState ( const uint8_t pdeviceId,
                                      const uint8_t state )
{
  if ( pdeviceId > 0 && pdeviceId <= N_POWER_DEVICES )
  {
    if ( pdeviceId - 1 < *numberOfPowerDevices )
    {
      powerDevice[pdeviceId - 1].state = state;
    }
  }
}

bool Opaq_storage::getPDevicePoint( const uint8_t stepId, const uint8_t value )
{
  for( int s=0; s < getPDeviceStepSize( id2idx(stepId) ); s += 2 )
  {
    if ( getPDeviceStep( id2idx(stepId), s ) <= value && value < getPDeviceStep( id2idx(stepId), s + 1 ) )
    {
      return true;
    }
  }
  
  return false;
}

void Opaq_storage::setPDesription( const uint8_t pidx, const char * desc )
{
  File psocketsFile = SPIFFS.open("/settings/psockets.json", "r+");

  if ( psocketsFile == NULL )
  {
    // create file
    psocketsFile = SPIFFS.open("/settings/psockets.json", "w");
  }
  
  // set position
  psocketsFile.seek( FILE_DESC_SIZE * pidx, SeekSet ) ;
  psocketsFile.write( (uint8_t*)desc, FILE_DESC_SIZE );
  psocketsFile.close();
}
*/




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

