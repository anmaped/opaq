
/*
 * Opaq_command: allows us to execute commands from diferent modules/plugins,
 * and recovery actions that can be taked outside of the opaq controller by
 * using the terminal.
 */

#include <ESPAsyncTCP.h>
#include <FS.h>

#include <ESP8266WiFi.h>

#include "Opaq_c1.h"
#include "Opaq_command.h"
#include "Opaq_storage.h"

#ifdef OPAQ_C1_SCREEN
#include "Opaq_iaqua.h"
#endif

#include "slre.h"
#include "version.h"

#include "zmodem.h"
#include "zmodem_config.h"
#include "zmodem_zm.h"

#define MAX_NUMBER_COMMAND_ARGS 6

std::atomic<bool> Opaq_command::ll;

Opaq_command command = Opaq_command();

namespace fnv1a_64 {

typedef std::uint64_t hash_t;

constexpr hash_t prime = 0x100000001B3ull;
constexpr hash_t basis = 0xCBF29CE484222325ull;

constexpr hash_t hash_compile_time(char const *str, hash_t last_value = basis) {
  return *str ? hash_compile_time(str + 1, (*str ^ last_value) * prime)
              : last_value;
}

hash_t hash(char const *str) {
  hash_t ret{basis};

  while (*str) {
    ret ^= *str;
    ret *= prime;
    str++;
  }

  return ret;
}

} // namespace fnv1a_64

constexpr unsigned long long operator"" _hash(char const *p, size_t) {
  return fnv1a_64::hash_compile_time(p);
}

void Opaq_command::begin() {}

void Opaq_command::end() {}

void Opaq_command::send(oq_cmd &cmd) {
  // lock to avoid concurrent adds

  lock();
  // enqueue command
  queue.add(cmd);
  unlock();
}

void Opaq_command::exec() {
  if (queue.size() != 0) {
    oq_cmd c;

    lock();
    c = queue.pop();
    unlock();

    c.exec(c.args);
  }
}

void Opaq_command::handler() {
  // for each queued command do
  for (unsigned short int i = 0; i < queue.size(); i++) {
    exec();
  }

  terminal();
}

void Opaq_command::terminal() {
  struct slre_cap caps[5];
  memset(caps, 0, 5);

  static String line = "";
  String lre = String(F("^([a-z0-9]+)(\\s[a-zA-Z0-9./-]*)*\\r|\\n$"));

  auto read_line = [](String &inData) {
    // define a timeout
    while (Serial.available() > 0) {
      char recieved = Serial.read();
      // echo
      Serial.write(recieved);

      if (recieved != '\b') {
        inData += recieved;
      } else {
        inData = inData.substring(0, inData.length() - 1);
      }

      if (recieved == '\n' || recieved == '\r') {
        Serial.write('\n');
        return true && (inData.length() > 0);
      }

      optimistic_yield(10000);
    }

    return false;
  };

  auto alert = [read_line]() {
    String in = F("");
    Serial.println(F("Are you sure? ([y]es/No):"));

    while (!read_line(in)) {
      optimistic_yield(10000);
    }

    if (in.substring(0, 1) == "y") {
      return true;
    } else {
      return false;
    }
  };

  auto list_files = []() {
    Dir d = SPIFFS.openDir("/");
    // list directory
    Serial.println(F("LIST DIR: "));
    while (d.next()) {
      String filename = d.fileName();
      Serial.printf(FF("%s %d  "), filename.c_str(), d.fileSize());
      Serial.println(F(""));
    }
  };

  auto split = [](String &input, String *out) {
    int counter = 0;
    int lastIndex = 0;

    for (int i = 0; i < input.length(); i++) {
      // Loop through each character and check if it's a space
      if (input.substring(i, i + 1) == " ") {
        // Grab the piece from the last index up to the current position and
        // store it
        out[counter] = input.substring(lastIndex, i);
        // Update the last position and add 1, so it starts from the next
        // character
        lastIndex = i + 1;
        // Increase the position in the array that we store into
        counter++;
        if (counter >= MAX_NUMBER_COMMAND_ARGS) {
          Serial.println("no more space.");
          break;
        }
      }

      // If we're at the end of the string (no more commas to stop us)
      if (i == input.length() - 1) {
        // Grab the last part of the string from the lastIndex to the end
        out[counter] = input.substring(lastIndex, i);
      }
    }
  };

  // get line otherwise exit
  if (!read_line(line))
    return;

  // lets parse the line
  if (slre_match(lre.c_str(), line.c_str(), line.length(), caps, 5, 0) > 0) {

    // Serial.print(F("opaq>"));
    // echo
    // Serial.println(line);

    String command = F("");
    command += caps[0].ptr;
    command.setCharAt(caps[0].len, '\0');
    DEBUG_MSG_COMMAND(FF("command=%s\r\n"), command.c_str());

    String args = "";
    String arg[MAX_NUMBER_COMMAND_ARGS] = {""};

    // static AsyncClient client = AsyncClient();

    switch (fnv1a_64::hash(command.c_str())) {
    case "format"_hash:
      if (alert()) {
        if (SPIFFS.format()) {
          Serial.println(F("Format successful"));
        } else {
          Serial.println(F("Format unsuccessful"));
        }
      }
      break;
    case "rm"_hash:
      args = caps[1].ptr;
      Serial.println(caps[1].len);
      args = args.substring(1);
      args.setCharAt(caps[1].len - caps[0].len - 1, '\0');
      Serial.println(args.c_str());

      if (SPIFFS.exists(args.c_str())) {
        Serial.println(F("Removed."));
        SPIFFS.remove(args.c_str());
      } else {
        Serial.println(F("Not found!"));
      }
      break;

    case "mv"_hash:
      args = caps[1].ptr;
      Serial.println(caps[1].len);
      args = args.substring(1);
      args.setCharAt(caps[1].len - caps[0].len, '\0');
      Serial.println(args.c_str());

      // args.split(' ');
      split(args, arg);

      Serial.println(arg[0]);
      Serial.println(arg[1]);

      SPIFFS.rename(arg[0].c_str(), arg[1].c_str());
      Serial.println(F("File renamed"));
      break;

    case "update"_hash: /*storage.fwupdate("noname.bin", "");*/
      args = caps[1].ptr;
      Serial.println(caps[1].len);
      args = args.substring(1);
      args.setCharAt(caps[1].len + caps[2].len - caps[0].len, '\0');
      Serial.println(args.c_str());

      split(args, arg);

      Serial.println(arg[0]);
      Serial.println(arg[1]);

      if (arg[0] == FF("avr")) {
        if (SPIFFS.exists(arg[1].c_str())) {
          communicate.lock();

          storage.avrprog.program(arg[1].c_str());

          communicate.unlock();
        } else {
          Serial.println(F("File does not found."));
        }
      } else if (arg[0] == FF("esp")) {

      } else {
        Serial.println(String() + FF("update of ") + arg[0] +
                       FF(" is not allowed."));
      }

      break;

    case "tar"_hash:
      args = caps[1].ptr;
      Serial.println(caps[1].len);
      args = args.substring(1);
      args.setCharAt(caps[1].len - caps[0].len, '\0');
      Serial.println(args.c_str());

      // args.split(' ');
      split(args, arg);

      Serial.println(arg[0]);
      Serial.println(arg[1]);
      // check inputs [todo]
      if (SPIFFS.exists(arg[0])) {
        storage.tarextract(arg[0].c_str(), arg[1].c_str());
        Serial.println(F("tar successful"));
      } else {
        Serial.println(F("tar unsuccessful"));
      }

      break;

    case "ls"_hash:
      list_files();
      break;

    case "setup"_hash:
      args = caps[1].ptr;
      args = args.substring(1);
      args.setCharAt(caps[1].len, '\0');
      split(args, arg);

#ifdef OPAQ_C1_SCREEN
      if (arg[1] == F("touch")) {
        if (arg[0] == F("set")) {
          // do calibration
          communicate.touch.doCalibration(tft_interface);
          if (communicate.touch.getCalibrationMatrix(
                  storage.touchsett.getTouchMatrixRef())) {
            storage.touchsett.commitTouchSettings();
            Serial.println(F("Touch settings has been updated."));
          } else {
            Serial.println(
                F("Touch settings generation has been failed. Reseting ..."));
          }
        } else {
          Serial.println(F("unknown touch command."));
        }
      }
#endif

      if (arg[1] == F("wifi")) {
        if (arg[0] == F("scan")) {
          WiFi.mode(WIFI_STA);
          WiFi.disconnect();
          delay(2000);

          Serial.println("scan start");

          // WiFi.scanNetworks will return the number of networks found
          WiFi.scanNetworks();
          while (WiFi.scanComplete() <= 0) {
            delay(100);
          }
          int n = WiFi.scanComplete();
          Serial.println("scan done");
          if (n == 0) {
            Serial.println("no networks found");
          } else {
            Serial.print(n);
            Serial.println(" networks found");
            for (int i = 0; i < n; ++i) {
              // Print SSID and RSSI for each network found
              Serial.print(i + 1);
              Serial.print(": ");
              Serial.print(WiFi.SSID(i));
              Serial.print(" (");
              Serial.print(WiFi.RSSI(i));
              Serial.print(")");
              Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " "
                                                                       : "*");
              delay(10);
            }
          }
          Serial.println("");
          break;
        }
        if (arg[0] == F("get")) {
          WiFi.printDiag(Serial);
          break;
        }
        if (arg[0] == F("set")) {
          if (args.length() > 5) {
            Serial.printf("mode:%s ssid:%s pwd:%s \n", arg[2].c_str(),
                          arg[3].c_str(), arg[4].c_str());
            Serial.println(F(""));
            if (arg[2] == F("AP")) {
              storage.wifisett.enableSoftAP();
              // [TODO]
            } else if (arg[2] == F("STA")) {
              storage.wifisett.setClientSSID(arg[3]);
              storage.wifisett.setClientPwd(arg[4]);
              storage.wifisett.enableClient();
            } else {
              ESP.eraseConfig();
              Serial.println("Wifi Settings clear.");
            }

            Serial.println(F("Reboot to apply settings."));
            break;
          }
        }
      }
      if (arg[1] == F("nrf24")) {
        if (arg[0] == F("get")) {
          communicate.nrf24.printstate();
          break;
        }
      }

      Serial.println(F("Usage: setup [get/set] [wifi/nrf24/touch]\r\n       "
                       "setup get wifi [example]"));

      break;

    case "mount"_hash:
      if (!SPIFFS.begin())
        Serial.println(F("Issues"));
      else
        Serial.println(F("Mounted"));
      break;

    case "rz"_hash:
      // SPIFFS.garbage();
      // wdt_enable(60000);
      // ets_wdt_disable();
      Serial.setDebugOutput(false);

      if (wcreceive(0, 0)) {
        Serial.println(F("zmodem transfer failed"));
      } else {
        Serial.println(F("zmodem transfer successful"));
      }

      Serial.setDebugOutput(true);
      break;

    case "reboot"_hash:
      ESP.restart();
      break;

    case "defrag"_hash:
      for (int i = 0; i < 100; i++)
        // SPIFFS.garbage();
        Serial.println(F("Filesystem cleanup."));
      break;

    case "dim"_hash:
      args = caps[1].ptr;
      Serial.println(caps[1].len);
      args = args.substring(1);
      args.setCharAt(caps[1].len - caps[0].len, '\0');
      Serial.println(args.c_str());

      communicate.tft_dimmer(atoi(args.c_str()));
      break;
    case "df"_hash:
      Serial.print(F("Disk Free: (bytes)\r\n"));
      FSInfo fs_info;
      SPIFFS.info(fs_info);
      Serial.print(F("Used -> "));
      Serial.println(fs_info.usedBytes);
      Serial.print(F("Total -> "));
      Serial.println(fs_info.totalBytes);
      break;
    case "free"_hash:
      Serial.print(F("Heap: "));
      Serial.println(ESP.getFreeHeap());
      break;
    case "stack"_hash:
      for (uint8_t i = 0; i < NUM_TASKS - 1; i++) {
        Serial.print(F("Task "));
        Serial.print(i);
        Serial.print(F(": "));
        Serial.print((uint32_t)stack_task[i]);
        Serial.print(F(" size: "));
        Serial.println(((uint32_t)stack_task[i]) -
                       ((uint32_t)stack_task[i + 1]));
      }

      break;

#ifdef OPAQ_C1_SCREEN
    case "test"_hash:
      args = caps[1].ptr;
      // Serial.println(caps[1].len);
      args = args.substring(1);
      args.setCharAt(caps[1].len, '\0');
      // Serial.println(args.c_str());
      split(args, arg);

      if (arg[0] == F("screen")) {
        communicate.lock();
        wscreen.draw();
        wscreen.msg(String(F("This is a testscreen")).c_str());
        communicate.unlock();

        for (uint8_t i = 0; i < 100; i++) {
          delay(100);
          communicate.lock();
          wscreen.setExecutionBar(i);
          communicate.unlock();
        }

        communicate.lock();
        wscreen.clear();
        testFilledRects(10000, 20000);
        communicate.unlock();
        break;
      }

      Serial.println("Usage: test [screen]");
      break;
#endif

    case "help"_hash:
      // show available commands
      Serial.println(F("Available commands: \r\n \
            update [avr/esp]- Update internal firmwares \r\n \
            format - Erase everything from SPIFFS filesystem \r\n \
            mount - mount SPIFFS filesystem \r\n \
            defrag - Restore delected and fragmented sectors \r\n \
            tar <file> - Extract tar archives \r\n \
            ls - List files \r\n \
            mv <file> <newfile> - Move files \r\n \
            rm <file> - Remove files \r\n \
            setup [set/get] [wifi/nrf24] - Explore and customize settings \r\n \
            dim <intensity> - Dim lcd 0-255 \r\n \
            test screen - Do a test screen \r\n \
            coprocessor [version] - Get a coproc status \r\n \
            rz - ZModem File Receiver \r\n \
            free - Show free memory \r\n \
            reboot - Soft reboot opaq \r\n \
            uname - Get version \r\n \
            help"));
      break;

    case "aws"_hash:
      // lets try to connect with ssl to aws iot
      /*client.onSslFileRequest([](void * arg, const char *filename, uint8_t
  **buf) -> int { Serial.printf("SSL File: %s\n", filename); File file =
  SPIFFS.open(filename, "r"); if(file){ size_t size = file.size(); uint8_t *
  nbuf = (uint8_t*)malloc(size); if(nbuf){ size = file.read(nbuf, size);
                      file.close();
                      *buf = nbuf;
                      return size;
              }
              file.close();
          }
          *buf = 0;
          return 0;
      }, NULL);

      client.onData([](void*arg, AsyncClient* c, void *data, size_t len){
          Serial.println("Data available.");
          Serial.println(len);
      },NULL);

      client.onConnect([](void* args, AsyncClient*client){
          Serial.println("SSL connected.");

          client->write("GET /hello.htm HTTP/1.1\r\n");
          client->write("User-Agent: Mozilla/4.0 (compatible; MSIE5.01;
  Windows NT)\r\n"); client->write("Host: www.google.com\r\n");
          client->write("Accept-Language: en-us\r\n");
          client->write("Accept-Encoding: gzip, deflate\r\n");
          client->write("Connection: Keep-Alive\r\n");
          client->write("\r\n");
      }, NULL);


  client.connect(FF("a3ha84sug66yhm.iot.eu-central-1.amazonaws.com"), 8883,
  true);*/
      break;

    case "uname"_hash:
      Serial.print(F("version: "));
      Serial.println(version);
      Serial.print(F("id: "));
      Serial.println(id);
      Serial.print("ESP8266 Core: ");
      Serial.println(ESP.getCoreVersion());
      break;

    case "coprocessor"_hash:
      args = caps[1].ptr;
      // Serial.println(caps[1].len);
      args = args.substring(1);
      args.setCharAt(caps[1].len, '\0');
      // Serial.println(args.c_str());
      split(args, arg);

      if (arg[0] == F("version")) {

        byte x[25];

        if (communicate.connect())
          return;

        SPI.transfer(ID_STATUS);
        delayMicroseconds(30);

        for (byte i = 0; i < 25; i++) {
          x[i] = SPI.transfer(0);
          delayMicroseconds(30);
        }

        communicate.disconnect();

        Serial.print("version: ");
        for (byte i = 1; i < 4; i++) {
          Serial.print((char)x[i]);
        }
        Serial.println("");

        Serial.print("id: ");
        for (byte i = 4; i < 25; i++) {
          Serial.print((char)x[i]);
        }
        Serial.println("");

        break;
      }

      Serial.println("Usage: coprocessor [version]");
      break;

#ifdef OPAQ_C1_SCREEN
    case "ui"_hash:
      args = caps[1].ptr;
      // Serial.println(caps[1].len);
      args = args.substring(1);
      args.setCharAt(caps[1].len, '\0');
      // Serial.println(args.c_str());
      split(args, arg);

      {
        auto eventlist = iaqua.get_eventlist();
        if (arg[0] == F("pop")) {
          if (eventlist->size() > 0) {
            auto el = eventlist->pop();
            Serial.print(F("UI event ("));
            Serial.print(eventlist->size());
            Serial.println(F("): "));
            serializeJson(el, Serial);
            Serial.println(F(""));
            break;
          } else {
            Serial.println(F("No events!"));
            break;
          }
        }

        if (arg[0] == F("push")) {

          StaticJsonDocument<128> doc;
          if (deserializeJson(doc, arg[1]) != DeserializationError::Ok) {
            Serial.println(F("Json parsing error."));
            break;
          }

          eventlist->add(doc);
          break;
        }
      }

      Serial.println("Usage: 'ui pop' or 'ui push <json_string>'");
      break;
#endif

    default:
      Serial.println(F("Unknown command."));
      break;
    }

    Serial.print(F("opaq>"));

  } else {
    // echo
    Serial.println(line);
    Serial.println(F("Unknown command."));
    Serial.print(F("opaq>"));
  }

  // reset string
  line = "";

  // activate handler to parse commands
}
