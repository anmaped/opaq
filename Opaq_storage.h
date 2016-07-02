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
 
#ifndef ACSTORAGE_H
#define ACSTORAGE_H

#include <Arduino.h>
#include <EEPROM.h>
#include <FS.h>
#include <ESP8266httpClient.h>
#include <ADS7846.h>

#define N_LIGHT_DEVICES 10
#define N_SIGNALS 3
#define N_POWER_DEVICES 20

// offsets for permanent storage
// SIGNATURE
#define OFFSET_SIGNATURE 0x00
#define SIGNATURE_LEN 1

// SSID for softAP mode
#define OFFSET_SSID OFFSET_SIGNATURE + SIGNATURE_LEN
#define SSID_LEN 32 + 1 // 1 for '\0' character

// PASSWORD for softAP mode
#define OFFSET_PWD OFFSET_SSID + SSID_LEN
#define PWD_LEN 32 + 1 // 1 for '\0' character

// SSID for CLIENT mode
#define OFFSET_C_SSID OFFSET_PWD + PWD_LEN
#define C_SSID_LEN 32 + 1 // 1 for '\0' character

// PASSWORD for CLIENT mode
#define OFFSET_C_PWD OFFSET_C_SSID + C_SSID_LEN
#define C_PWD_LEN 64 + 1 // 1 for '\0' character

// modes os operation (first bit means CLIENT or softAP)
#define OFFSET_SET OFFSET_C_PWD + C_PWD_LEN
#define SET_LEN 1

// number of light devices
#define OFFSET_NLD OFFSET_SET + SET_LEN //(OFFSET_LD + LD_LEN)
#define NLD_LEN 1

// light device settings
#define OFFSET_LD OFFSET_NLD + NLD_LEN
#define LD_LEN_EACH (sizeof(Opaq_storage::deviceLightDescriptor))
#define LD_LEN (LD_LEN_EACH * N_LIGHT_DEVICES)

#define S_LEN_EACH 32 + 1 // a multiple of two + 1 byte for storing the number of points

// number of power devices
#define OFFSET_NPD OFFSET_LD + LD_LEN
#define NPD_LEN 1

// power devices settings
#define OFFSET_PD OFFSET_NPD + NPD_LEN
#define PD_LEN_EACH (sizeof(Opaq_storage::deviceDescriptorPW))
#define PD_LEN (PD_LEN_EACH * N_POWER_DEVICES)

// offset of the end of the permanent storage
#define MAXIMUM_SETTINGS_STORAGE OFFSET_PD + PD_LEN

// code length for light devices
#define LIGHT_CODE_ID_LENGTH 5

#define FILE_DESC_SIZE 32

// conversion from id to index, and vice-versa
#define id2idx(id) id-1
#define idx2id(idx) idx+1

#define SIGNAL_STEP_SIZE 2
#define SIGNAL_LENGTH ( (S_LEN_EACH - 1) / SIGNAL_STEP_SIZE )
#define check_deviceIdx(deviceIdx) ( 0 <= deviceIdx && deviceIdx <  N_LIGHT_DEVICES )
#define check_signalIdx(signalIdx) ( 0 <= signalIdx && signalIdx <  N_SIGNALS )
#define check_pointId(pointId) ( 0 < pointId && pointId <=  SIGNAL_LENGTH )

// enumerations for light and power devices settings
enum type {OPENAQV1 = 1, ZETLIGHT_LANCIA_2CH};
enum ptype {CHACON_DIO = 1, OTHERS};
enum pstate {OFF = 0, ON, BINDING, UNBINDING, ON_PERMANENT, OFF_PERMANENT, AUTO, LISTENING};


class Opaq_storage: EEPROMClass
{
private:

    struct device_descriptor
    {
        uint8_t id;
        uint8_t type;
        uint8_t codeid[LIGHT_CODE_ID_LENGTH];
        pstate state;
        bool linear;
        uint8_t signal[N_SIGNALS][S_LEN_EACH];
    } __attribute__ ( ( packed ) );


    struct pdeviceDesciptor
    {
        uint8_t id;
        uint8_t type;
        uint8_t state;
        uint32_t code;
        /* step is enconded by the clock value and the respectite boolean value;
           the value of the last element indicates the number of steps */
        uint8_t step[S_LEN_EACH];
    } __attribute__ ( ( packed ) );


    uint8_t* flashPointer;
    uint8_t* signature;
    char* ssid;
    char* pwd;
    char* c_ssid;
    char* c_pwd;
    uint8_t* op;

    struct device_descriptor* lightDevice;
    struct pdeviceDesciptor* powerDevice;

    uint8_t* numberOfLightDevices;
    uint8_t cLidx_device;
    uint8_t* numberOfPowerDevices;

    // state variables for touch matrix calibration
    CAL_MATRIX touch_cal_matrix;
    CAL_MATRIX& touch_cal_matrix_ref = touch_cal_matrix;

public:

    typedef struct device_descriptor deviceLightDescriptor;
    typedef struct pdeviceDesciptor deviceDescriptorPW;

    Opaq_storage();

    /* restore default settings */
    void defaults ( uint8_t sig );
    /* store current settings */
    void save();
    void close();

    void initOpaqC1Service();

    /* getters for device structure arrays and their sizes */
    deviceLightDescriptor* getLightDevices() { return lightDevice; };
    unsigned int getNumberOfLightDevices() { return *numberOfLightDevices; };

    uint8_t getNLDevice() { return *numberOfLightDevices; };
    uint8_t getLDeviceId ( uint8_t idx ) { return lightDevice[idx].id; };
    uint8_t getLDeviceSignal ( const uint8_t x, const uint8_t y ) { return lightDevice[getCurrentSelLDevice()].signal[x][y]; };
    uint8_t getLDeviceSignal ( const uint8_t idx, const uint8_t x,
                               const uint8_t y ) { return lightDevice[idx].signal[x][y]; };
    pstate getLState( const uint8_t id ) { return id-1 < 0 ? OFF : lightDevice[id-1].state; };
    void setLState( const uint8_t idx, pstate st ) { lightDevice[idx].state = st; };
    uint8_t * getCodeId( const uint8_t idx ) { return lightDevice[id2idx(idx)].codeid; };

    uint8_t getLDeviceSignalLength( const uint8_t deviceIdx, const uint8_t signalIdx ) { return lightDevice[deviceIdx].signal[signalIdx][S_LEN_EACH-1]; };
    void setLDeviceSignalLength( const uint8_t deviceIdx, const uint8_t signalIdx, const uint8_t value ) { lightDevice[deviceIdx].signal[signalIdx][S_LEN_EACH-1] = value; };

    uint8_t getCurrentSelLDevice() { return cLidx_device; };
    void selectLDevice ( const uint8_t idx ) { cLidx_device = idx; };


    uint8_t getPDeviceStep ( const uint8_t pidx, const uint8_t st ) { return powerDevice[pidx].step[st]; };
    uint8_t getPDeviceStepSize ( const uint8_t pidx ) { return powerDevice[pidx].step[S_LEN_EACH-1]; };
    deviceDescriptorPW* getPowerDevices() { return powerDevice; };
    unsigned int getNumberOfPowerDevices() { return *numberOfPowerDevices; };

    void setPDeviceStep( const uint8_t pid, const uint8_t st, const uint8_t value ) { powerDevice[pid].step[st] = value; };
    void setPDesription( const uint8_t pidx, const char * desc );

    /* sig symbol for eeprom verification */
    const uint8_t getSignature();

    /* request SSID for acess point operation */
    const char* getSSID() { return ( const char* ) ssid; };
    const char* getPwd() { return ( const char* ) pwd; };
    const char* getClientSSID() { return ( const char* ) c_ssid; };
    const char* getClientPwd() { return ( const char* ) c_pwd; };
    void setClientSSID( const char* ssid ) { if( strlen(ssid) < C_SSID_LEN ) memcpy ( c_ssid, ssid, strlen(ssid) );  };
    void setClientPwd( const char* pwd ) { if( strlen(pwd) < C_PWD_LEN ) memcpy ( c_pwd, pwd, strlen(pwd) ); };

    /* get settings for operating as access point or client */
    uint8_t getModeOperation() { return *op; };
    void enableSoftAP() { *op |= 0x1; };
    void enableClient() { *op &= ~0x1; };

    /* light device management functions */
    void addLightDevice();
    void addSignal ( uint8_t deviceId, uint8_t signalId, uint8_t pointId,
                     uint8_t xy, uint8_t value );
    bool setPointXLD (  uint8_t deviceIdx, uint8_t signalIdx, uint8_t pointId, uint8_t value )
    {
      if ( check_deviceIdx(deviceIdx) && check_signalIdx(signalIdx) && check_pointId(pointId) )
        lightDevice[deviceIdx].signal[signalIdx][pointId * SIGNAL_STEP_SIZE - 2] = value;
    };
    bool setPointYLD (  uint8_t deviceIdx, uint8_t signalIdx, uint8_t pointId, uint8_t value )
    {
      if ( check_deviceIdx(deviceIdx) && check_signalIdx(signalIdx) && check_pointId(pointId) )
      lightDevice[deviceIdx].signal[signalIdx][pointId * SIGNAL_STEP_SIZE - 1] = value;
    };

    void getLinearInterpolatedPoint ( uint8_t deviceId, uint8_t signalId,
                                      float value, uint8_t* y );

    void setDeviceType ( const uint8_t deviceId, const uint8_t type );
    uint8_t getDeviceType ( uint8_t );

    /* power socket functions */
    void addPowerDevice();
    void setPowerDeviceState ( const uint8_t pdeviceId, const uint8_t state );

    uint32_t getPDeviceCode ( uint8_t pdeviceId ) { return powerDevice[pdeviceId - 1].code; }; // unsafe
    uint8_t getPDeviceState ( uint8_t pdeviceId ) { return powerDevice[pdeviceId - 1].state; }; // unsafe
    bool getPDevicePoint( const uint8_t stepId, const uint8_t value );


    // touch screen settings
    CAL_MATRIX& getTouchMatrixRef();
    CAL_MATRIX getTouchMatrix();
    void commitTouchSettings();
    bool isTouchMatrixAvailable();

};

extern Opaq_storage storage;

class PrintFile : public Print
{
  File stream;
  
public:
  PrintFile(const char* filename) { stream = SPIFFS.open(filename, "w+"); };
  ~PrintFile() { stream.close(); };
  size_t write(uint8_t b) { stream.write(b); };
};


#endif // ACSTORAGE_H
