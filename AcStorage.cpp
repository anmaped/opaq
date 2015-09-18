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
 
#include "AcStorage.h"

#include <user_interface.h>
#include <mem.h>

AcStorage::AcStorage() : EEPROMClass()
{
  // initialize eeprom
  begin ( SPI_FLASH_SEC_SIZE );

  // store flash pointer
  flashPointer = getDataPtr();

  // align variables with flash address
  signature = ( uint8_t* ) flashPointer + OFFSET_SIGNATURE;
  ssid = ( char* ) flashPointer + OFFSET_SSID;
  pwd = ( char* ) flashPointer + OFFSET_PWD;
  op = ( uint8_t* ) flashPointer + OFFSET_SET;

  lightDevice = reinterpret_cast<deviceLightDescriptor*> ( flashPointer +
                OFFSET_LD );
  numberOfLightDevices = ( uint8_t* ) flashPointer + OFFSET_NLD;
  cLidx_device = 0;

  powerDevice = reinterpret_cast<deviceDescriptorPW*> ( flashPointer +
                OFFSET_PD );

  numberOfPowerDevices = ( uint8_t* ) flashPointer + OFFSET_NPD;
}

void AcStorage::defauls ( uint8_t sig )
{
  for ( int i = 0; i < SPI_FLASH_SEC_SIZE; i++ ) {flashPointer[i] = 0x00;}

  *signature = sig;

  // SSID setting
  static const char* lssid PROGMEM = "opaq-AAAA";
  memcpy ( ssid, lssid, SSID_LEN );

  // default PWD
  static const char* lpwd PROGMEM = "1234567890";
  memcpy ( pwd, lpwd, PWD_LEN );

  // operation modes for controller
  // access point mode enabled - bit 0b---- ---X
  *op = 0b00000001;

  *numberOfLightDevices = 0;

  *numberOfPowerDevices = 0;

}

void AcStorage::save()
{
  if ( commit() != true )
    Serial.println ( "storage commit was NOT sucessfull." );
}

const uint8_t AcStorage::getSignature()
{
  return ( const uint8_t ) * signature;
}

uint8_t AcStorage::getModeOperation()
{
  return *op;
}

void AcStorage::addLightDevice()
{
  // defensive case
  if ( *numberOfLightDevices >= N_LIGHT_DEVICES )
    return;

  /*const deviceLightDescriptor device = {
    .id = (uint8_t)((*numberOfLightDevices)+((uint8_t)1)),
    .type = OPENAQV1,
    .codeid = {0x00},
    .linear = true,
    .signal = {0x00},
  };*/


  //memcpy(((uint8_t*)lightDevice) + (*numberOfLightDevices*N_LIGHT_DEVICES), &device, LD_LEN_EACH);
  //memset(&lightDevice[*numberOfLightDevices], 0, LD_LEN_EACH);

  lightDevice[*numberOfLightDevices].id = *numberOfLightDevices + 1;
  lightDevice[*numberOfLightDevices].type = OPENAQV1;
  lightDevice[*numberOfLightDevices].linear = true;

  //lightDevice[*numberOfLightDevices].signal = (((uint8_t*)lightDevice) + 13);
  /*for(int i=0; i<N_SIGNALS; i++)
  {
    for(int j=0; j<S_LEN_EACH; j++)
      lightDevice[*numberOfLightDevices].signal[i][j] = 1;
  }*/

  ( *numberOfLightDevices )++;

  Serial.println ( *numberOfLightDevices );
  Serial.println ( LD_LEN_EACH );
  //Serial.println(system_get_free_heap_size());
}

void AcStorage::addSignal ( uint8_t deviceId, uint8_t signalId, uint8_t pointId,
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

void AcStorage::getLinearInterpolatedPoint ( uint8_t deviceId, uint8_t signalId,
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

void AcStorage::setDeviceType ( const uint8_t deviceId, const uint8_t type )
{
  if ( deviceId > 0 )
  {
    lightDevice[deviceId - 1].type = type;
  }
}

uint8_t AcStorage::getDeviceType ( uint8_t deviceId )
{
  if ( deviceId > 0 && deviceId <= N_LIGHT_DEVICES )
    return lightDevice[deviceId - 1].type;

  return 0;
}

void AcStorage::addPowerDevice()
{
  // defensive case
  if ( *numberOfPowerDevices >= N_POWER_DEVICES )
    return;

  powerDevice[*numberOfPowerDevices].id = *numberOfPowerDevices + 1;
  powerDevice[*numberOfPowerDevices].type = CHANON_DIO;
  powerDevice[*numberOfPowerDevices].state = OFF;
  powerDevice[*numberOfPowerDevices].code = random ( 0x1, 0x3ffffff );

  ( *numberOfPowerDevices )++;
}

void AcStorage::setPowerDeviceState ( const uint8_t pdeviceId,
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


