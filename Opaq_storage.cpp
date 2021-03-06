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
#include "Opaq_command.h"
#include "interpolate.h"

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <HashMap.h>

#include <libtar.h>

Opaq_storage storage = Opaq_storage();

Opaq_storage::Opaq_storage()
    : faqdim(Opaq_st_plugin_faqdim()), pwdevice(Opaq_st_plugin_pwdevice()),
      wifisett(Opaq_st_plugin_wifisett()),
      touchsett(Opaq_st_plugin_touchsett()), avrprog(328, 2) {}

void Opaq_storage::defaults(uint8_t sig) {
  // let's create the wifi configuration file
  // with: signature; ssid; pwd; clientssid; clientpwd; state; wifimode

  // SPIFFS.format();
  // SPIFFS.begin();

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

void Opaq_storage::initOpaqC1Service() {
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
  client.requestFile("http://ec2-52-29-83-128.eu-central-1.compute.amazonaws.com/opaq/c1/page/jquery.mobile-1.4.5.min.js",
  storer );

  // request mobile jquery file


  // get default opaq c1 settings
  */

  auto download_file = [](const char *filename) {
    HTTPClient http;

    File f = SPIFFS.open((String("/tmp/") + filename).c_str(), "w");

    if (!f) {
      return;
    }

    Serial.print(F("[HTTP] begin...\n"));
    Serial.println(filename);

    // configure server and url
    http.begin(
        (String(F("http://")) + OPAQ_URL_FIRMWARE_UPLOAD + F("/") + filename)
            .c_str());

    Serial.print(F("[HTTP] GET...\n"));
    // start connection and send HTTP header
    int httpCode = http.GET();

    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf(FF("[HTTP] GET... code: %d\n"), httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {

        // get lenght of document (is -1 when Server sends no Content-Length
        // header)
        int len = http.getSize();

        // create buffer for read
        uint8_t buff[1536] = {0};

        // get tcp stream
        WiFiClient *stream = http.getStreamPtr();

        // read all data from server
        while (http.connected() && (len > 0 || len == -1)) {
          // get available data size
          size_t size = stream->available();

          if (size) {
            // read up to 1536 byte
            int c = stream->readBytes(
                buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

            // write it to Serial
            // Serial.write(buff, c);
            f.write(buff, c);

            if (len > 0) {
              len -= c;
            }
          }

          yield();
        }

        Serial.println();
        Serial.print(F("[HTTP] connection closed or file end.\n"));

        f.close();
      }
    } else {
      Serial.printf(FF("[HTTP] GET... failed, error: %s\n"),
                    http.errorToString(httpCode).c_str());
    }

    http.end();
  };

  /*download_file("oaq_sett_fadimmer.html");
  download_file("oaq_sett_pdev.html");
  download_file("opaqc1.html");
  download_file("opaqc1_index.html");
  download_file("opaqc1_settings.html");*/

  // libtar_list("/www/www.tar");

  /*SPIFFS.end();

  SPIFFS.format();

  SPIFFS.begin();
*/

  // SPIFFS.format();

  // download file to get the lastest firmware and system files for opaq
  download_file(FF("opaqc1.json"));

  // parseConfiguration()

  String filename = "";
  String md5 = "";
  Opaq_st_plugin_dummy dummy;

  if (SPIFFS.exists(FF("/tmp/opaqc1.json"))) {
    dummy.parse(FF("/tmp/opaqc1.json"), FF("fw"), filename);
    dummy.parse(FF("/tmp/opaqc1.json"), FF("md5"), md5);

    download_file(filename.c_str());

    if (SPIFFS.exists(String("/tmp/") + filename)) {
      oq_cmd c;
      c.exec = [](LinkedList<String> args) {
        storage.fwupdate(args.pop().c_str(), args.pop().c_str());
      };
      c.args = LinkedList<String>();
      String filepath = String(F("/tmp/")) + filename;
      c.args.add(md5);
      c.args.add(filepath);

      command.send(c);
    }
  }

  // libtar_extract("/tmp/www.tar", "/www");

  // SPIFFS.remove("/tmp/www.tar");
}

void Opaq_storage::tarextract(const char *filename, const char *target) {
  libtar_extract(filename, target);

  SPIFFS.remove(filename);
}

void Opaq_storage::fwupdate(const char *filename, const char *md5) {
  // test if file exists
  if (!SPIFFS.exists(filename))
    return;

  File f = SPIFFS.open(filename, "r");

  if (!f)
    return;

  // get fw size
  const unsigned int chunck_size = FLASH_SECTOR_SIZE;
  uint8_t *buf = new uint8_t[chunck_size];
  int size = f.size();

  if (!Update.begin(size, U_FLASH)) {
    Serial.println(F("Update failed!"));
    return;
  }

  if (strlen(md5)) {
    if (!Update.setMD5(md5)) {
      Serial.println(F("MD5 failed!"));
    }
  }

  for (int i = 0; i < size; i += chunck_size) {
    size_t ss = f.read(buf, chunck_size);

    if (Update.write(buf, ss) != ss) {
      f.close();
      return;
    }
  }

  f.close();

  delete buf;

  // lets remove the unnecessary file
  SPIFFS.remove(filename);

  if (!Update.end()) {
    Serial.println(F("Update end failed!"));
  } else {
    ESP.restart();
  }
}

const uint8_t Opaq_storage::getSignature() {
  byte sig;

  File fl = SPIFFS.open(FF("/sett/sig"), "r");

  if (fl) {
    sig = fl.read();
    fl.close();
  }

  return sig;
}

void Opaq_storage::writeSignature(byte sig) {
  File fl = SPIFFS.open(FF("/sett/sig"), "w");

  if (fl) {
    fl.write(sig);
    fl.close();
  }
}

File &FileIterator::operator*() {
  last_file = _p_vec->dirlist().openFile("r");
  return last_file;
}

const FileIterator &FileIterator::operator++() {
  last_file.close();

  if (!_p_vec->dirlist().next()) {
    go_on = false;
  }

  return *this;
}

void Opaq_st_plugin::load(File &fl, String &toParse) {
  // json to mem
  uint8_t buf[64];
  int len, global_len;

  global_len = fl.size();

  while ((len = fl.read(buf, 64)) != 0) {
    buf[len] = '\0';
    toParse += (char *)buf;
  }

  DEBUG_MSG_STORAGE(FF("%s\n"), toParse);
}

bool Opaq_st_plugin::load(const char *filename, String &toParse) {
  if (!SPIFFS.exists(filename))
    return true;

  DEBUG_MSG_STORAGE(FF("Loading..."));

  File fl = SPIFFS.open(filename, "r+");

  load(fl, toParse);

  fl.close();

  return false;
}

void Opaq_st_plugin::parse(const char *filename, const char *param,
                           String &out) {
  String toParse;
  StaticJsonDocument<512> doc;

  DEBUG_MSG_STORAGE(FF("%d\n"), param);

  if (!load(filename, toParse)) {
    deserializeJson(doc, toParse);

    out = (const char *)doc[param];

    DEBUG_MSG_STORAGE(FF("out: %s\n"), out);
  }
}

void Opaq_st_plugin_wifisett::parseConfiguration(const char *param,
                                                 String &out) {
  parse(FF("/sett/wifi/conf.json"), param, out);
}

void Opaq_st_plugin_wifisett::changeConfiguration(const char *param,
                                                  const char *value) {
  String toParse, tmp = F("/sett/wifi/conf.json");
  StaticJsonDocument<512> doc;

  if (!load(tmp.c_str(), toParse))
    deserializeJson(doc, toParse);

  doc[param] = value;

  // save file
  PrintFile file = PrintFile(tmp.c_str(), "w");
  serializeJson(doc, file);
}

void Opaq_st_plugin_wifisett::defaults() {
  char tmp[30];
  sprintf(tmp, FF("opaq-%08x"), ESP.getChipId());

  setSSID(tmp);
  setPwd(FF("opaqopaq"));

  enableSoftAP();
}

void Opaq_st_plugin_wifisett::getSSID(String &ssid) {
  parseConfiguration(FF("wssid"), ssid);
}

void Opaq_st_plugin_wifisett::getPwd(String &pwd) {
  parseConfiguration(FF("wpwd"), pwd);
}

void Opaq_st_plugin_wifisett::getClientSSID(String &ssid) {
  parseConfiguration(FF("wclientssid"), ssid);
}

void Opaq_st_plugin_wifisett::getClientPwd(String &pwd) {
  parseConfiguration(FF("wclientpwd"), pwd);
}

void Opaq_st_plugin_wifisett::getMode(String &mode) {
  parseConfiguration(FF("wmode"), mode);
}

void Opaq_st_plugin_wifisett::setSSID(String ssid) {
  changeConfiguration(FF("wssid"), ssid.c_str());
}

void Opaq_st_plugin_wifisett::setPwd(String pwd) {
  changeConfiguration(FF("wpwd"), pwd.c_str());
}

void Opaq_st_plugin_wifisett::setClientSSID(String ssid) {
  changeConfiguration(FF("wclientssid"), ssid.c_str());
}

void Opaq_st_plugin_wifisett::setClientPwd(String pwd) {
  changeConfiguration(FF("wclientpwd"), pwd.c_str());
}

uint8_t Opaq_st_plugin_wifisett::getModeOperation() {
  String tmp;
  parseConfiguration(FF("wmode"), tmp);

  return tmp == FF("softAP");
}

void Opaq_st_plugin_wifisett::enableSoftAP() {
  changeConfiguration(FF("wmode"), FF("softAP"));
}

void Opaq_st_plugin_wifisett::enableClient() {
  changeConfiguration(FF("wmode"), FF("client"));
}

void Opaq_st_plugin_faqdim::defaults() {}

void Opaq_st_plugin_faqdim::add() {
  DynamicJsonDocument doc(2048);
  char code[5 * 2 + 1];
  String filename;

  // try to find a valid id
  do {
    // let's generate the id
    for (byte i = 0; i < 10; i += 2) {
      sprintf(&code[i], "%02X", random(0x0, 0xff));
    }
    code[5 * 2] = '\0';

    getFilename(filename, code);
  } while (SPIFFS.exists(filename.c_str()));

  // lets create the empty file
  File tmp = SPIFFS.open(filename.c_str(), "w");
  tmp.close();

  // printer for json
  PrintFile file = PrintFile(filename.c_str());

  // light device properties
  // (codeID, type, description, state)
  doc[F("adimid")] = code;
  doc[F("type")] = (unsigned int)OPENAQV1;
  doc[F("description")] = code;
  doc[F("state")] = "on";
  doc[F("cursor")] = 4; // the size of the default data

  // [['00:00',1],['01:00',1] ...]
  JsonArray data = doc.createNestedArray("data");

  // number of channels
  for (byte j = 0; j < 4; j++) {
    JsonArray subdata = data.createNestedArray();

    for (byte i = 0; i < 24; i++) {
      JsonArray subsubdata = subdata.createNestedArray();

      subsubdata.add(i * 60 * 60 * 1000);
      subsubdata.add(j * 10 + i);
    }
  }

  doc[F("size")] = doc.size();
  serializeJson(doc, file);
}

void Opaq_st_plugin_faqdim::save(const char *code, const uint8_t *content,
                                 size_t len) {
  String filename;

  getFilename(filename, code);

  Serial.println(F("FILE SAVE:"));
  Serial.println(filename);

  // file to save
  File tmp = SPIFFS.open(filename, "w");

  tmp.write(content, len);

  tmp.close();
}

void Opaq_st_plugin_faqdim::getDir(String &filename) {
  filename = F("/sett/adim/");
}

void Opaq_st_plugin_faqdim::getFilename(String &filename, const char *code) {
  getDir(filename);
  filename += code;
  filename += F(".json");
}

void Opaq_st_plugin_faqdim::remove(const char *code) {
  String filename = "";

  getFilename(filename, code);

  // for each file in /sett/adim directory do
  Dir directory = SPIFFS.openDir(FF("/sett/adim"));

  while (directory.next()) {
    if (directory.fileName() == filename) {
      Serial.println(F("ADIM FILE REMOVED"));
      SPIFFS.remove(directory.fileName());
    }
  }
}

void Opaq_st_plugin_faqdim::run() {
  DynamicJsonDocument doc(2048);
  String tmp;

  // get clock
  RtcDateTime date;
  communicate.getClock(date);
  unsigned long clock_value =
      (date.Second() + date.Minute() * 60 + date.Hour() * 60 * 60) * 1000;

  auto getState = [](DynamicJsonDocument &obj, unsigned long clk,
                     LinkedList<byte> &signal) {
    // get array
    JsonArray arr = obj[F("data")];

    // create linked list for each elements
    for (int i = 0; arr[i].is<JsonArray>(); i++) {
      LinkedList<std::pair<unsigned long, byte>> signallist;

      JsonArray subarr = arr[i];

      for (int j = 0; subarr[j].is<JsonArray>(); j++) {
        JsonArray subsubarr = subarr[j];

        unsigned long time_ms = subsubarr[0];
        byte value = subsubarr[1];

        DEBUG_MSG_STORAGE(FF("ac:: adim %d, %d %d\r\n"), i, time_ms, value);

        auto pa = std::make_pair(time_ms, value);
        signallist.add(pa);
      }

      byte new_value =
          hermiteInterpolate<unsigned long, byte>(signallist, clk, 0, 0);

      DEBUG_MSG_STORAGE(FF("ac:: adim %d\r\n"), new_value);

      signal.add(new_value);
    }
  };

  // for each aquarium dimmer device
  // probably is not safe at all (files can be removed... concurrent accesses)
  Directory dr = Directory(FF("/sett/adim"));
  for (File fl : dr) {
    if (fl == NULL)
      continue;

    Serial.println(fl.size());

    // get fl to memory to parse it
    tmp = ""; // [TODO] put file to buffer

    load(fl, tmp);

    

    if (deserializeJson(doc, tmp) != DeserializationError::Ok)
      break;

    // convert code to integer
    unsigned long code = strtol((const char *)doc[F("adimid")], NULL, 16);
    String state = String((const char *)doc[F("state")]);
    String type = String(
        (const char *)doc[F("type")]); // [TODO] restrict that by type

    DEBUG_MSG_STORAGE(FF("ac:: adim %d %s %lu\r\n"), code, state.c_str(),
                      clock_value);

    // store computed signal values according to the clock value
    LinkedList<byte> signalstate_list;

    // for zetlancia case
    if (type == FF("zetlancia")) {
      tmp_type = ZETLIGHT_LANCIA_2CH;
    }

    // check if some device is binding...
    if (state == FF("bind")) {
      send(code, NRF24_BINDING);
    }

    if (state == FF("unbind")) {
      send(code, NRF24_UNBINDING);
    }

    if (state == FF("listen")) {
      send(code, NRF24_LISTENING);
    }

    if (state == FF("auto")) {
      getState(doc, clock_value, signalstate_list);
      send(code, signalstate_list);
    } else if (state == FF("on")) {
      send(code, NRF24_ON);
    } else if (state == FF("off")) {
      send(code, NRF24_OFF);
    }

    optimistic_yield(10000);
  }
}

void Opaq_st_plugin_faqdim::send(unsigned int code, nrf24state state) {
  RF24 &radio = communicate.nrf24.getRF24();

  auto calculate_lrc = [](uint8_t *buf) {
    // calculate LRC - longitudinal redundancy check
    uint8_t LRC = 0;

    for (int j = 1; j < 14; j++) {
      LRC ^= buf[j];
    }

    return LRC;
  };

  if (tmp_type == ZETLIGHT_LANCIA_2CH) {
    uint8_t *codepointer = (uint8_t *)&code;

    communicate.lock();

    if (state == NRF24_BINDING) {
      // binding message
      uint8_t binding_data[32] = {
          //<3byte static signature>
          0xEE, 0x0D, 0x0A,
          // BINDING mode
          //                                            <N_MODE>             <
          //                                            binding mode > <groupId>
          //                                            <LRC>
          0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x23, 0xdc, 0x0, 0x01, 0x00,
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
          0x00, 0x00, // 0x08, 0x06 -  0x7b, 0x15,
          //<unknown values>
          0x0, 0x0};

      binding_data[14] = calculate_lrc(binding_data);

      radio.stopListening();
      // DEBUGV("ac:: BIND msg sent\n");

      radio.setChannel(100);
      delayMicroseconds(10000);

      memcpy(&binding_data[19], &codepointer[0], 4);

      binding_data[28] = 0x7B;
      binding_data[29] = 0x15;

      radio.startWrite(binding_data, 32, true);

      delayMicroseconds(10000);
      radio.setChannel(1);
    }

    if (state == NRF24_LISTENING) {
      // read...

      // radio.setChannel(100);
      uint8_t buf[32];
      bool a = true;
      String x = F("{\"nrf24M\" :[");

      if (radio.available()) {
        radio.read(buf, 32);

        char tmp[10];

        for (int ib = 0; ib < 32; ib++) {
          sprintf(tmp, FF("\"%02x\"%c "), buf[ib], ib < 31 ? ',' : ' ');
          x += tmp;
        }
        x += F("]}");

        a = false;
      }

      radio.startListening();

      if (a == false) {
        a = true;
        Serial.println(x);
        //[TODO] put that to output ... webSocket.sendTXT(0, x.c_str(),
        //x.length());
      }
    }

    communicate.unlock();
  }
}

void Opaq_st_plugin_faqdim::send(unsigned int code, LinkedList<byte> &state) {
  RF24 &radio = communicate.nrf24.getRF24();

  auto calculate_lrc = [](uint8_t *buf) {
    // calculate LRC - longitudinal redundancy check
    uint8_t LRC = 0;

    for (int j = 1; j < 14; j++) {
      LRC ^= buf[j];
    }

    return LRC;
  };

  if (tmp_type == ZETLIGHT_LANCIA_2CH) {
    communicate.lock();

    uint8_t *codepointer = (uint8_t *)&code;

    if (state.size() < 2) {
      return;
    }

    float tmp = ((float)state.get(0)) * (255 / 100);
    uint8_t signal1 = tmp;
    tmp = ((float)state.get(1)) * (255 / 100);
    uint8_t signal2 = tmp;

    Serial.println(F("SIGNAL"));
    Serial.println(signal1);
    Serial.println(signal2);

    /*{"nrf24M" :["ee", "0d", "0a", "00", "91", "00", "9d", "10", "00", "00",
      "00", "00", "00", "00", "1b", "ec", "00", "00", "00", "e2", "0b", "f5",
      "37", "c0", "00", "00", "68", "71", "00", "00", "00", "00"  ]}*/

    // normal message
    uint8_t buf[32] = {//<3byte static signature>
                       0xEE, 0xD, 0xA,
                       // <------------------------------ LIGHT
                       // -------------------------------------------------->
                       //  <Mode>= DAM(0x03); SUNRISE(0xb6); DAYTIME(0xb6);
                       //  SUNSET(0xD4); NIGHTTIME(0x06)
                       //  <Mode2>= DAM(0x00); SUNRISE(0x15); DAYTIME(0x58);
                       //  SUNSET(0x2A); NIGHTIME(0x2A)
                       // Normal mode (N_MODE)
                       //<Mode1>     <value1>  <Mode2>    <value2>   <N_MODE> <
                       //binding mode >  <groupId>    <LRC>
                       0x00, signal1, 0x00, signal2, 0x10, 0x0, 0x0, 0x0, 0x0,
                       0x0, 0x0, 0x00,
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
                       0x00, 0x0, // means nothing
                       //<unknown values>
                       0x0, 0x0};

    buf[14] = calculate_lrc(buf);

    radio.stopListening();
    // DEBUGV("ac:: ON msg sent\n");

    memcpy(&buf[19], &codepointer[0], 4);

    radio.startWrite(buf, 32, true);

    communicate.unlock();
  }
}

void Opaq_st_plugin_pwdevice::defaults() {}
void Opaq_st_plugin_pwdevice::add() {
  DynamicJsonDocument doc(2048);
  char code[7 + 1];
  String filename;

  const int nchan = 1;

  // try to find a valid id
  do {
    // let's generate the id
    sprintf(&code[0], "%07X", random(0x1, 0x3ffffff));
    code[7] = '\0';

    getFilename(filename, code);
  } while (SPIFFS.exists(filename.c_str()));

  // lets create the empty file
  File tmp = SPIFFS.open(filename.c_str(), "w");
  tmp.close();

  // printer for json
  PrintFile file = PrintFile(filename.c_str());

  // power device properties
  // (codeID, type, description, state)
  doc[F("pdevid")] = code;
  doc[F("type")] = (unsigned int)CHACON_DIO;
  doc[F("description")] = code;
  doc[F("state")] = "auto";
  doc[F("cursor")] = nchan; // the size of the default data

  // [['00:00',1],['01:00',1] ...]
  JsonArray data = doc.createNestedArray("data");

  // number of channels
  for (byte j = 0; j < nchan; j++) {
    JsonArray subdata = data.createNestedArray();

    bool inv = false;
    for (byte i = 0; i < 24; i++) {
      JsonArray subsubdata = subdata.createNestedArray();

      subsubdata.add(i * 60 * 60 * 1000);
      subsubdata.add(inv);
      inv = !inv;
    }
  }

  doc[F("size")] = doc.size();
  serializeJson(doc, file);
}

void Opaq_st_plugin_pwdevice::save(const char *code, const uint8_t *content,
                                   size_t len) {
  String filename;

  getFilename(filename, code);

  Serial.println(F("FILE SAVE:"));
  Serial.println(filename);

  // file to save
  File tmp = SPIFFS.open(filename, "w");

  tmp.write(content, len);

  tmp.close();
}

void Opaq_st_plugin_pwdevice::getDir(String &filename) {
  filename = F("/sett/pdev/");
}

void Opaq_st_plugin_pwdevice::getFilename(String &filename, const char *code) {
  getDir(filename);
  filename += code;
  filename += F(".json");
}

void Opaq_st_plugin_pwdevice::remove(const char *code) {
  String filename = "";

  getFilename(filename, code);

  // for each file in /sett/adim directory do
  Dir directory = SPIFFS.openDir(FF("/sett/pdev"));

  while (directory.next()) {
    if (directory.fileName() == filename) {
      Serial.println(F("PDEV FILE REMOVED"));
      SPIFFS.remove(directory.fileName());
    }
  }
}

void Opaq_st_plugin_pwdevice::run() {
  DynamicJsonDocument doc(2048);
  String tmp;

  auto getState = [](DynamicJsonDocument &obj, unsigned long clk) {
    // get array
    JsonArray arr = obj[F("data")];

    // create linked list for each elements
    for (int i = 0; arr[i].is<JsonArray>(); i++) {
      LinkedList<std::pair<unsigned long, byte>> signallist;

      JsonArray subarr = arr[i];

      for (int j = 0; subarr[j].is<JsonArray>(); j++) {
        JsonArray subsubarr = subarr[j];

        unsigned long time_ms = subsubarr[0];
        byte value = subsubarr[1];

        DEBUG_MSG_STORAGE(FF("ac:: pdev %d, %d %d\r\n"), i, time_ms, value);

        auto pa = std::make_pair(time_ms, value);
        signallist.add(pa);
      }

      byte new_value =
          hermiteInterpolate<unsigned long, byte>(signallist, clk, 0, 0);

      DEBUG_MSG_STORAGE(FF("ac:: pdev %d\r\n"), new_value);
    }

    return RF433_ON;
  };

  // get clock
  RtcDateTime date;
  communicate.getClock(date);
  unsigned long clock_value =
      (date.Second() + date.Minute() * 60 + date.Hour() * 60 * 60) * 1000;

  // probably is not safe at all (files can be removed... concurrent accesses)
  Directory dr = Directory(FF("/sett/pdev"));
  for (File fl : dr) {
    if (fl == NULL)
      continue;

    Serial.println(fl.size());

    // get fl to memory to parse it
    tmp = ""; // [TODO] put file to buffer

    load(fl, tmp);

    if (deserializeJson(doc, tmp) != DeserializationError::Ok)
      break;

    // convert code to integer
    long code = strtol((const char *)doc[F("pdevid")], NULL, 16);
    String state = String((const char *)doc[F("state")]);
    String type = String(
        (const char *)doc[F("type")]); // [TODO] restrict that by type

    DEBUG_MSG_STORAGE(FF("ac:: pdev %d %s %lu\r\n"), code, state.c_str(),
                      clock_value);

    // for chacon dio case
    if (type != FF("chacondio"))
      break;

    // check if some device is binding...
    if (state == FF("bind")) {
      send(code, RF433_BINDING);
    }

    if (state == FF("unbind")) {
      send(code, RF433_UNBINDING);
    }

    if (state == FF("auto")) {
      send(code, getState(doc, clock_value));
    } else if (state == FF("on")) {
      send(code, RF433_ON);
    } else if (state == FF("off")) {
      send(code, RF433_OFF);
    }

    optimistic_yield(10000);
  }
}

void Opaq_st_plugin_pwdevice::send(unsigned int code,
                                   short unsigned int state) {
  static CreateHashMap(statemap, unsigned int, byte, 50);

  byte tmp[10];
  byte idx = 0;

  if (statemap.contains(code) && state == statemap[code] &&
      state != RF433_BINDING && state != RF433_UNBINDING) {
    return;
  }

  statemap[code] = state;

  // comunicate with coprocessor
  Serial.println(F("ASK AVR TO RECEIVE RF433 STREAM"));

  if (communicate.connect())
    return;

  tmp[idx++] = SPI.transfer(ID_RF433_STREAM);
  delayMicroseconds(30);

  // payload length
  tmp[idx++] = SPI.transfer(0x05);
  delayMicroseconds(30);

  tmp[idx++] = SPI.transfer((byte)code);
  delayMicroseconds(30);
  tmp[idx++] = SPI.transfer((byte)(code >> 8));
  delayMicroseconds(30);
  tmp[idx++] = SPI.transfer((byte)(code >> 16));
  delayMicroseconds(30);
  tmp[idx++] = SPI.transfer((byte)(code >> 24));
  delayMicroseconds(30);
  tmp[idx++] = SPI.transfer(state);
  delayMicroseconds(30);

  // dummy
  tmp[idx++] = SPI.transfer(0x10);
  delayMicroseconds(30);

  communicate.disconnect();

  for (byte i = 0; i < idx; i++)
    Serial.println(String(F("r: ")) + String(tmp[i]));

  Serial.println(F("AVR ASKED!"));
}

void Opaq_st_plugin_touchsett::defaults() {}

bool Opaq_st_plugin_touchsett::isTouchMatrixAvailable() {
  return SPIFFS.exists(FF("/sett/touch.json"));
}

CAL_MATRIX &Opaq_st_plugin_touchsett::getTouchMatrixRef() {
  return touch_cal_matrix_ref;
}

CAL_MATRIX Opaq_st_plugin_touchsett::getTouchMatrix() {
  StaticJsonDocument<400> doc;
  String toParse;
  uint8_t buf[64];
  int len, global_len;

  // refresh settings from the file system
  File touch_settings_file = SPIFFS.open(FF("/sett/touch.json"), "r+");
  // touch_settings_file.read();
  touch_settings_file.seek(0, SeekEnd);
  global_len = touch_settings_file.position();
  touch_settings_file.seek(0, SeekSet);

  // test if there is free memory available
  // [TODO]

  while ((len = touch_settings_file.read(buf, 64)) != 0) {
    buf[len] = '\0';
    toParse += (char *)buf;
  }

  touch_settings_file.close();

  Serial.println(toParse);

  deserializeJson(doc, toParse);

  touch_cal_matrix.a = doc[F("a")];
  touch_cal_matrix.b = doc[F("b")];
  touch_cal_matrix.c = doc[F("c")];
  touch_cal_matrix.d = doc[F("d")];
  touch_cal_matrix.e = doc[F("e")];
  touch_cal_matrix.f = doc[F("f")];
  touch_cal_matrix.div = doc[F("div")];
  touch_cal_matrix.endpoints[MIN_ENDPOINT].x = doc[F("mix")];
  touch_cal_matrix.endpoints[MAX_ENDPOINT].x = doc[F("max")];
  touch_cal_matrix.endpoints[MIN_ENDPOINT].y = doc[F("miy")];
  touch_cal_matrix.endpoints[MAX_ENDPOINT].y = doc[F("may")];

  return touch_cal_matrix;
}

void Opaq_st_plugin_touchsett::commitTouchSettings() {
  StaticJsonDocument<400> doc;
  PrintFile file = PrintFile(FF("/sett/touch.json"), "w");

  doc[F("a")] = touch_cal_matrix.a;
  doc[F("b")] = touch_cal_matrix.b;
  doc[F("c")] = touch_cal_matrix.c;
  doc[F("d")] = touch_cal_matrix.d;
  doc[F("e")] = touch_cal_matrix.e;
  doc[F("f")] = touch_cal_matrix.f;
  doc[F("div")] = touch_cal_matrix.div;
  doc[F("mix")] = touch_cal_matrix.endpoints[MIN_ENDPOINT].x;
  doc[F("max")] = touch_cal_matrix.endpoints[MAX_ENDPOINT].x;
  doc[F("miy")] = touch_cal_matrix.endpoints[MIN_ENDPOINT].y;
  doc[F("may")] = touch_cal_matrix.endpoints[MAX_ENDPOINT].y;

  serializeJson(doc, file);
}
