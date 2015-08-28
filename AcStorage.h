
#ifndef ACSTORAGE_H
#define ACSTORAGE_H

#include <Arduino.h>

#define N_LIGHT_DEVICES 3
#define N_SIGNALS 3

// offsets for permanent storage
#define OFFSET_SSID 0x00
#define SSID_LEN 16

#define OFFSET_PWD OFFSET_SSID + SSID_LEN
#define PWD_LEN 11 + 1 // 1 for '\0' character

#define OFFSET_SET OFFSET_PWD + PWD_LEN
#define SET_LEN 4

#define OFFSET_NLD OFFSET_SET + SET_LEN //(OFFSET_LD + LD_LEN)
#define NLD_LEN 4

#define OFFSET_LD OFFSET_NLD + NLD_LEN
#define LD_LEN_EACH (sizeof(AcStorage::deviceLightDescriptor))
#define LD_LEN (LD_LEN_EACH * N_LIGHT_DEVICES)

#define S_LEN_EACH 32 + 1 // a multiple of two + 1 byte for storing the number of points




enum type {OPENAQV1=1, ZETLIGHT_LANCIA_2CH};

class AcStorage {
  private:
  
    struct device_descriptor {
      uint8_t id;
      uint8_t type;
      uint8_t codeid[10];
      bool linear;
      uint8_t signal[N_SIGNALS][S_LEN_EACH];
    };
    
    uint8_t *flashPointer;
    
    
        
    char* ssid;
    char* pwd;
    uint8_t* op;
    struct device_descriptor* lightDevice;//[N_LIGHT_DEVICES];
    
    uint8_t* numberOfLightDevices;
  
  public:
  //uint8_t* arr;
    typedef struct device_descriptor deviceLightDescriptor;
    
    
    AcStorage();
    
    void defauls();
    void save();
    
    deviceLightDescriptor* getLightDevices(){return lightDevice;};
    
    unsigned int getNumberOfLightDevices(){return *numberOfLightDevices;};
    
    const char* getSSID();
    
    uint8_t getModeOperation();
    
    void addLightDevice();
    void addSignal(uint8_t deviceId, uint8_t signalId, uint8_t pointId, uint8_t xy, uint8_t value);
    
    void getLinearInterpolatedPoint(uint8_t deviceId, uint8_t signalId, float value, uint8_t *y);

    void setDeviceType(const uint8_t deviceId, const uint8_t type);
    uint8_t getDeviceType(uint8_t);
    
};


#endif // ACSTORAGE_H
