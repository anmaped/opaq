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
 
#ifndef ACSTORAGE_H
#define ACSTORAGE_H

#include <Arduino.h>
#include <FS.h>
#include <LinkedList.h>

#include "src/ADS7846/ADS7846.h"
#include "opaq.h"
#include "Opaq_coprogrammer.h"


#ifdef DEBUG_ESP_OPAQSTORAGE
#define DEBUG_MSG_STORAGE(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#else
#define DEBUG_MSG_STORAGE(...) 
#endif


// enumerations for light and power devices settings
enum pdevtype {CHACON_DIO = 1, OTHERS};
enum nrf24state {NRF24_OFF = 0, NRF24_ON, NRF24_BINDING, NRF24_UNBINDING, NRF24_LISTENING};


// forward-declaration to allow use in Iter
class Directory;
 
class FileIterator
{
    public:
    FileIterator (const Directory* p_vec) :
        _p_vec( p_vec ),
        go_on(true)
    {
    }

    ~FileIterator()
    {
      if (last_file != NULL)
      {
        last_file.close();
      }
    }
 
    bool
    operator!= (const FileIterator& other) const
    {
        return go_on && _p_vec != NULL;
    }
 
    File& operator* ();
    const FileIterator& operator++ ();
 
    private:
    const Directory *_p_vec;
    File last_file;
    bool go_on;
};

class Directory
{
    public:
    Directory (const char * filename)
    {
      dir = SPIFFS.openDir(filename);
      can_run = dir.next();
    }

    FileIterator begin () const
    {
        return FileIterator( (can_run)? this : NULL );
    }
 
    FileIterator end () const
    {
        return FileIterator( NULL );
    }

    Dir& dirlist() const
    {
      return dir_ref;
    }
 
    private:
    Dir dir;
    Dir& dir_ref = dir;
    bool can_run;
};


class PrintFile : public Print
{
  File stream;
  
public:
  PrintFile(const char* filename, const char * fo = "r+") { stream = SPIFFS.open(filename, fo); };
  ~PrintFile() { stream.close(); };
  size_t write(uint8_t b) { stream.write(b); };
  File& getStream() { return stream; };
};

class Opaq_st_plugin
{
public:
  virtual void defaults() = 0;
  bool load(const char * filename, String& toParse);
  void load(File& fl, String& toParse);
  void parse(const char * filename, const char * param, String& out);
};

class Opaq_st_plugin_dummy : public Opaq_st_plugin
{
  void defaults() {};
};

class Opaq_st_plugin_faqdim : public Opaq_st_plugin
{
  enum adimtype {OPENAQV1 = 1, ZETLIGHT_LANCIA_2CH, UNKNOWN};
  adimtype tmp_type;
public:

  void defaults();
  
  void add();
  void save(const char * code, const uint8_t * content, size_t len);
  void getDir(String& filename);
  void getFilename(String& filename, const char * code);
  void remove(const char* code);  

  void run();
  void send(unsigned int code, nrf24state state);
  void send(unsigned int code, LinkedList<byte>& state);
};


class Opaq_st_plugin_pwdevice : public Opaq_st_plugin
{
public:

  void defaults();
  
  void add();
  void save(const char * code, const uint8_t * content, size_t len);
  void getDir(String& filename);
  void getFilename(String& filename, const char * code);
  void remove(const char* code);

  void run();
  void send(unsigned int code, short unsigned int state);
};

class Opaq_st_plugin_wifisett : public Opaq_st_plugin
{
  void parseConfiguration(const char * param, String& out);
  void changeConfiguration(const char * param, const char * value);
public:

  void defaults();
  
  /* request SSID for acess point operation */
  void getSSID(String& ssid);
  void getPwd(String& pwd);
  void getClientSSID(String& ssid);
  void getClientPwd(String& pwd);
  void getMode(String& mode);
  void setSSID( String ssid );
  void setPwd( String pwd );
  void setClientSSID( String ssid );
  void setClientPwd( String pwd );

  /* get settings for operating as access point or client */
  uint8_t getModeOperation();
  void enableSoftAP();
  void enableClient();

  
};

class Opaq_st_plugin_touchsett : public Opaq_st_plugin
{
private:

  // state variables for touch matrix calibration
  CAL_MATRIX touch_cal_matrix;
  CAL_MATRIX& touch_cal_matrix_ref = touch_cal_matrix;
  
public:

  void defaults();
  
  // touch screen settings
  CAL_MATRIX& getTouchMatrixRef();
  CAL_MATRIX getTouchMatrix();
  void commitTouchSettings();
  bool isTouchMatrixAvailable();
};

class Opaq_storage
{
private:
  bool update;
public:

    Opaq_storage();

    /* restore default settings */
    void defaults ( uint8_t sig );

    void initOpaqC1Service();
    void tarextract(const char * filename, const char * target);
    void fwupdate(const char * filename, const char * md5);
    bool getUpdate() { return update; };
    void setUpdate(bool state) { update = state; };

    /* sig symbol for eeprom verification */
    const uint8_t getSignature();
    void writeSignature(byte sig);


    // static initialized for all plugins
    Opaq_st_plugin_faqdim     faqdim;
    Opaq_st_plugin_pwdevice   pwdevice;
    Opaq_st_plugin_wifisett   wifisett;
    Opaq_st_plugin_touchsett  touchsett;

    Opaq_coprogrammer avrprog;

};

extern Opaq_storage storage;

#endif // ACSTORAGE_H
