
#include "AcStorage.h"

#include <user_interface.h>
#include <mem.h>

#include <EEPROM.h>

AcStorage::AcStorage()
{
  // initialize eeprom
  EEPROM.begin(SPI_FLASH_SEC_SIZE);
  
  //uint8_t * arr;
  //arr = (uint8_t *) os_malloc(1024);
  //os_memset(arr, 0, 1024);
  //static uint8_t arr[1024] = {0x00};
  //arr = new uint8_t[1024];
  
  // store flash pointer
  flashPointer = EEPROM.getDataPtr();
  
  // align variables with flash address
  ssid = (char *) flashPointer + OFFSET_SSID;
  pwd = (char *) flashPointer + OFFSET_PWD;
  op = (uint8_t *) flashPointer + OFFSET_SET;
  lightDevice = reinterpret_cast<deviceLightDescriptor *>(flashPointer + OFFSET_LD);
  numberOfLightDevices = (uint8_t *) flashPointer + OFFSET_NLD;
}

void AcStorage::defauls()
{
  for (int i=0; i < 1024; i++) {flashPointer[i] = 0x00;}
  
  // SSID setting
  static const char* lssid PROGMEM = "OpenAq-AAAA";
  memcpy(ssid, lssid, SSID_LEN);
  
  // default PWD
  static const char* lpwd PROGMEM = "1234567890";
  memcpy(pwd, lpwd, PWD_LEN);
  
  // operation modes for controller
  // access point mode enabled - bit 0b---- ---X
  *op=0b00000001;
  
  *numberOfLightDevices=0;
  
  // initialization for list of devices
  //os_memset(lightDevice, 0, LD_LEN);  
}

void AcStorage::save()
{
  EEPROM.commit();
}
  
const char* AcStorage::getSSID()
{
  return (const char*) ssid;
}

uint8_t AcStorage::getModeOperation()
{
  return *op;
}

void AcStorage::addLightDevice()
{ 
  // defensive case
  if (*numberOfLightDevices >= N_LIGHT_DEVICES)
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
    
  (*numberOfLightDevices)++;
  
  Serial.println(*numberOfLightDevices);
  Serial.println(LD_LEN_EACH);
  //Serial.println(system_get_free_heap_size());
}

void AcStorage::addSignal(uint8_t deviceId, uint8_t signalId, uint8_t pointId,  uint8_t xy, uint8_t value)
{
  // make checks [TODO]
  if (deviceId > 0 && signalId > 0 && pointId > 0 && value <= 255)
  {
    if (xy == 0)
    {
      lightDevice[deviceId-1].signal[signalId-1][pointId*2-2] = value;
    }
    else
    {
      lightDevice[deviceId-1].signal[signalId-1][pointId*2-1] = value;
    }
  }
}

void AcStorage::getLinearInterpolatedPoint(uint8_t deviceId, uint8_t signalId, float x, uint8_t *y)
{
  uint8_t x0=25, y0=25, x1=25, y1=25;
  bool finded=false;

  x = (x/24*255);
  
  // find two points where value is between them
  if ( deviceId > 0 && signalId > 0)
  {
    if ( lightDevice[deviceId-1].signal[signalId-1][S_LEN_EACH-1] > 0)
    {
      for ( int i=0; i < lightDevice[deviceId-1].signal[signalId-1][S_LEN_EACH-1]-1; i++)
      {
        if (lightDevice[deviceId-1].signal[signalId-1][i*2 + 1] <= x && x < lightDevice[deviceId-1].signal[signalId-1][(i+1)*2 + 1])
        {
          y0 = lightDevice[deviceId-1].signal[signalId-1][i*2];
          x0 = lightDevice[deviceId-1].signal[signalId-1][i*2 + 1];
          y1 = lightDevice[deviceId-1].signal[signalId-1][(i+1)*2];
          x1 = lightDevice[deviceId-1].signal[signalId-1][(i+1)*2 + 1];
          finded=true;
          break;
        }
      }
      
      if (finded)
      {
        *y = y0 + ((uint8_t)(((float)(y1-y0)) * (((float)(x-x0))/((float)(x1-x0)))));
        DEBUGV("::li %d %d %d %d %d %d \r\n", x0, y0, x1, y1, (unsigned int )x, *y);
      }
      else
      {
        DEBUGV("::li not found\r\n");
        *y = 26;
      }
    }
    else
    {
      *y = 25;
    }
  }
}

void AcStorage::setDeviceType(const uint8_t deviceId, const uint8_t type)
{
  if ( deviceId > 0 )
  {
    lightDevice[deviceId-1].type = type;
  }
}

uint8_t AcStorage::getDeviceType(uint8_t deviceId)
{
  if (deviceId > 0)
    return lightDevice[deviceId-1].type;

  return 0;
}
  

