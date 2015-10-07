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

#define N_LIGHT_DEVICES 10
#define N_SIGNALS 3
#define N_POWER_DEVICES 20

// offsets for permanent storage
#define OFFSET_SIGNATURE 0x00
#define SIGNATURE_LEN 1

#define OFFSET_SSID OFFSET_SIGNATURE + SIGNATURE_LEN
#define SSID_LEN 16

#define OFFSET_PWD OFFSET_SSID + SSID_LEN
#define PWD_LEN 11 + 1 // 1 for '\0' character

#define OFFSET_SET OFFSET_PWD + PWD_LEN
#define SET_LEN 1

#define OFFSET_NLD OFFSET_SET + SET_LEN //(OFFSET_LD + LD_LEN)
#define NLD_LEN 1

#define OFFSET_LD OFFSET_NLD + NLD_LEN
#define LD_LEN_EACH (sizeof(AcStorage::deviceLightDescriptor))
#define LD_LEN (LD_LEN_EACH * N_LIGHT_DEVICES)

#define S_LEN_EACH 32 + 1 // a multiple of two + 1 byte for storing the number of points

#define OFFSET_NPD OFFSET_LD + LD_LEN
#define NPD_LEN 1

#define OFFSET_PD OFFSET_NPD + NPD_LEN
#define PD_LEN_EACH (sizeof(AcStorage::deviceDescriptorPW))
#define PD_LEN (PD_LEN_EACH * N_POWER_DEVICES)

#define MAXIMUM_SETTINGS_STORAGE OFFSET_PD + PD_LEN

#define LIGHT_CODE_ID_LENGTH 5

#define id2idx(id) id-1

enum type {OPENAQV1 = 1, ZETLIGHT_LANCIA_2CH};

enum ptype {CHACON_DIO = 1, OTHERS};

enum pstate {OFF = 0, ON, BINDING, UNBINDING};

class AcStorage: EEPROMClass
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
    uint8_t* op;

    struct device_descriptor* lightDevice;
    struct pdeviceDesciptor* powerDevice;

    uint8_t* numberOfLightDevices;
    uint8_t cLidx_device;
    uint8_t* numberOfPowerDevices;

public:

    typedef struct device_descriptor deviceLightDescriptor;
    typedef struct pdeviceDesciptor deviceDescriptorPW;

    AcStorage();

    /* restore default settings */
    void defauls ( uint8_t sig );
    /* store current settings */
    void save();

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

    uint8_t getCurrentSelLDevice() { return cLidx_device; };
    void selectLDevice ( const uint8_t idx ) { cLidx_device = idx; };


    uint8_t getPDeviceStep ( const uint8_t pid, const uint8_t st ) { return powerDevice[pid].step[st]; };
    deviceDescriptorPW* getPowerDevices() { return powerDevice; };
    unsigned int getNumberOfPowerDevices() { return *numberOfPowerDevices; };

    void setPDeviceStep( const uint8_t pid, const uint8_t st, const uint8_t value ) { powerDevice[pid].step[st] = value; };

    /* sig symbol for eeprom verification */
    const uint8_t getSignature();

    /* request SSID for acess point operation */
    const char* getSSID() { return ( const char* ) ssid; };
    const char* getClientSSID() { return ( const char* ) ""; };
    const char* getClientPwd() { return ( const char* ) ""; };

    /* get settings for operating as access point or client */
    uint8_t getModeOperation();

    /* light device management functions */
    void addLightDevice();
    void addSignal ( uint8_t deviceId, uint8_t signalId, uint8_t pointId,
                     uint8_t xy, uint8_t value );

    void getLinearInterpolatedPoint ( uint8_t deviceId, uint8_t signalId,
                                      float value, uint8_t* y );

    void setDeviceType ( const uint8_t deviceId, const uint8_t type );
    uint8_t getDeviceType ( uint8_t );

    /* power socket functions */
    void addPowerDevice();
    void setPowerDeviceState ( const uint8_t pdeviceId, const uint8_t state );

    uint32_t getPDeviceCode ( uint8_t pdeviceId ) { return powerDevice[pdeviceId - 1].code; }; // unsafe
    uint8_t getPDeviceState ( uint8_t pdeviceId ) { return powerDevice[pdeviceId - 1].state; }; // unsafe

};


#endif // ACSTORAGE_H
