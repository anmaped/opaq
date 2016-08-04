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
 
#include "Opaq_storage.h"

#include <user_interface.h>
#include <mem.h>
#include <ArduinoJson.h>


Opaq_storage storage = Opaq_storage();

static_assert( !(MAXIMUM_SETTINGS_STORAGE >= SPI_FLASH_SEC_SIZE) , "SETTINGS EXCEED ALLOWED SIZE");

extern "C" uint32_t _SPIFFS_end;
Opaq_storage::Opaq_storage() : EEPROMClass((((uint32_t)&_SPIFFS_end - 0x40200000) / SPI_FLASH_SEC_SIZE))
{
  faqdim = Opaq_st_plugin_faqdim ();
   
  // initialize eeprom
  begin ( SPI_FLASH_SEC_SIZE );

  // store flash pointer
  flashPointer = getDataPtr();

  // align variables with flash address
  signature = ( uint8_t* ) flashPointer + OFFSET_SIGNATURE;
  ssid = ( char* ) flashPointer + OFFSET_SSID;
  pwd = ( char* ) flashPointer + OFFSET_PWD;
  c_ssid = ( char* ) flashPointer + OFFSET_C_SSID;
  c_pwd = ( char* ) flashPointer + OFFSET_C_PWD;
  op = ( uint8_t* ) flashPointer + OFFSET_SET;

  lightDevice = reinterpret_cast<deviceLightDescriptor*> ( flashPointer +
                OFFSET_LD );
  numberOfLightDevices = ( uint8_t* ) flashPointer + OFFSET_NLD;
  cLidx_device = 0;

  powerDevice = reinterpret_cast<deviceDescriptorPW*> ( flashPointer +
                OFFSET_PD );

  numberOfPowerDevices = ( uint8_t* ) flashPointer + OFFSET_NPD;
}

void Opaq_storage::defaults ( uint8_t sig )
{
  for ( int i = 0; i < SPI_FLASH_SEC_SIZE; i++ ) { flashPointer[i] = 0x00; }

  *signature = sig;

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
  *numberOfPowerDevices = 0;

  SPIFFS.format();
  SPIFFS.begin();

  save();

  ESP.restart();
  
}

void Opaq_storage::save()
{
  if ( commit() != true )
    Serial.println ( "storage commit was NOT sucessfull." );
}

void Opaq_storage::close()
{
  close();
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
}

const uint8_t Opaq_storage::getSignature()
{
  return ( const uint8_t ) * signature;
}

void Opaq_st_plugin_faqdim::add()
{
  /*// defensive case
  if ( *numberOfLightDevices >= N_LIGHT_DEVICES )
    return;

  lightDevice[*numberOfLightDevices].id = *numberOfLightDevices + 1;
  lightDevice[*numberOfLightDevices].type = OPENAQV1;
  lightDevice[*numberOfLightDevices].linear = true;
  lightDevice[*numberOfLightDevices].state = ON;
  
  for( int i=0; i < LIGHT_CODE_ID_LENGTH; i++ )
  {
    lightDevice[*numberOfLightDevices].codeid[i] = random ( 0x0, 0xff );
  }

  ( *numberOfLightDevices )++;

  Serial.println ( *numberOfLightDevices );
  Serial.println ( LD_LEN_EACH );
*/

  DynamicJsonBuffer jsonBuffer;
  char code[5*2+1];
  String filename;

  // try to find a valid id
  do
  {
     // let's generate the id
    for(byte i=0; i< 5; i+=2)
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
  root["description"] = "Device";
  root["state"] = (unsigned int)ON;

  // [['00:00',1],['01:00',1] ...]
  JsonArray& data = root.createNestedArray("data");

  // number of channels
  for(byte j=0; j<4; j++)
  {
     JsonArray& subdata = data.createNestedArray();

    for(byte i=0; i < 24; i++)
    {
      JsonArray& subsubdata = subdata.createNestedArray();
  
      char tmp[10];
      if (i>9)
        sprintf(tmp, "%d:00", i);
      else
        sprintf(tmp, "0%d:00", i);
      
      subsubdata.add(jsonBuffer.strdup(tmp));
      subsubdata.add(j*10+i);
    }
  
  }

  root["pointer"] = file.getStream().size()-1;
  root["size"] = 3000;
  
  root.printTo(file);
}

void Opaq_st_plugin_faqdim::save(const char * code, const uint8_t * content, size_t len)
{ 
  String filename;
  
  getFilename(filename, code);

  Serial.println("FILE SAVE:");
  Serial.println(filename);
  
  // file to save
  File tmp = SPIFFS.open(filename, "r+");

  tmp.write(content, len);
  
  tmp.close();
}

void Opaq_st_plugin_faqdim::getFilename(String& filename, const char * code)
{
  filename = "";
  filename += F("/sett/adim/");
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
      Serial.println("FILE REMOVED");
      SPIFFS.remove(directory.fileName());
    }
  }
}

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


bool Opaq_storage::isTouchMatrixAvailable()
{
  return SPIFFS.exists("/sett/touch.json");
}

CAL_MATRIX& Opaq_storage::getTouchMatrixRef()
{ 
  return touch_cal_matrix_ref;
}

CAL_MATRIX Opaq_storage::getTouchMatrix()
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

void Opaq_storage::commitTouchSettings()
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

