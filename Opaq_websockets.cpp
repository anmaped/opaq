
#include "Opaq_websockets.h"
#include "Opaq_storage.h"
#include "Opaq_c1.h"

#include "AsyncJson.h"
#include "ArduinoJson.h"

void sendFile( fs::FS fs, const char * name, String& content)
{
  uint8_t stmp[32 + 1];
  
  // open file
  File tmp = fs.open(name, "r");
  
  if(tmp != NULL)
  {
    // get content to memory
    for(int i=0; i < tmp.size(); i+=32)
    {
      size_t sz = tmp.read(stmp, 32);
      stmp[sz] = '\0';
      content += (char *)stmp;
    }
    
    tmp.close();
  }

}

void parseTextMessage(AsyncWebSocketClient * client, uint8_t * data, size_t len)
{
  DynamicJsonBuffer jsonBuffer;
  
  // parse possible json files
  if( data[0] =='{' && data[len - 1] == '}' )
  {
     // let's parse the json file
     Serial.println(F("JSON STR RECEIVED!"));

     char tmp[len + 1];
     memcpy(tmp, (char *)data, len);
     tmp[len] = '\0';
     JsonObject& root = jsonBuffer.parseObject(tmp);

     // ########################
     // Wifi configuration file
     // ########################
     if ( root.containsKey(FF("mode")) )
     {
  
       if ( strcmp((const char*)root[F("mode")], FF("ap")) == 0 )
       {
         Serial.println(F("AP"));
         storage.wifisett.setSSID((const char*)root[F("ssid")]);
         storage.wifisett.setPwd((const char*)root[F("pwd")]);

         storage.wifisett.enableSoftAP();
       }
       else if ( strcmp((const char*)root[F("mode")], FF("sta")) == 0 )
       {
         Serial.println(F("STA"));
         storage.wifisett.setClientSSID((const char*)root[F("ssid")]);
         storage.wifisett.setClientPwd((const char*)root[F("pwd")]);
  
         // set mode to STA
         storage.wifisett.enableClient();
       }
       
     }



     // #########################
     // Set Clock
     // #########################
     if (root.containsKey(FF("setclock")) )
     {
       JsonObject& rootsetclock = root.get(FF("setclock"));
      
       if ( rootsetclock.containsKey("type") && ( strcmp((const char*)rootsetclock[F("type")], FF("ntp")) == 0 ) )
       {
         // get ntp time
         opaq_controller.syncClock();
       }
       else
       {
         // get values from json
         RtcDateTime tmp = RtcDateTime( rootsetclock[F("year")], rootsetclock[F("month")], rootsetclock[F("day")], rootsetclock[F("hour")], rootsetclock[F("minute")], rootsetclock[F("second")]);
         opaq_controller.getCom().setClock(tmp);
       }
     }

     // #########################
     // Get Clock
     // #########################
     if (root.containsKey(FF("getclock")) )
     {
       // let's send the clock values
       JsonObject& tmp_root = jsonBuffer.createObject();

       JsonObject& conf = tmp_root.createNestedObject("realtimeclock");

       RtcDateTime date;
       opaq_controller.getCom().getClock(date); // [TODO] this can fail
       
       conf[F("second")] = date.Second();
       conf[F("minute")] = date.Minute();
       conf[F("hour")]   = date.Hour();
       conf[F("day")]    = date.Day();
       conf[F("month")]  = date.Month();
       conf[F("year")]   = date.Year();

       String tmp = F("");
       tmp_root.printTo(tmp);

       client->text(tmp);
     }


     
     // ################
     // Filename getter
     // ################
     if (root.containsKey(FF("filename")) )
     {
       String content = "";
       
       sendFile(SPIFFS, root[F("filename")], content);
       
       client->text(content);
     }

     // ################################
     // Light device full dimmer setter
     // ###############################
     if (root.containsKey(FF("adimid")) )
     {
       // let's write the file configuration
      
       storage.faqdim.save(root[F("adimid")], data, len);
      
       client->text(FF("{\"success\":\"\"}"));
     }

     // ################################
     // Light device full dimmer getter
     // ################################
     if (root.containsKey(FF("adim")) )
     {
       String content = "", dirname = "";
       
       // list devices and send it
       // {"adim":["filea.json","fileb.json"]}
       
       content += F("{\"adim\":[\"\"");

       storage.faqdim.getDir(dirname);
       
       // for each file in /sett/adim directory do
       Dir directory = SPIFFS.openDir(dirname.c_str());
       
       while ( directory.next() )
       {
         content += F(", \"");
         content += directory.fileName();
         content += F("\" ");
       }

       content += F(" ]}");
   
       client->text(content);
    }

    // ##################################
    // Light device full dimmer remover
    // #################################
    if (root.containsKey(FF("adimremove")) )
    {
      storage.faqdim.remove(root[F("adimremove")]);
     
      client->text(FF("{\"success\":\"\"}"));
    }

    // ################################
    // Light device full dimmer setter
    // ################################
    if (root.containsKey(FF("adimadd")) )
    {
      storage.faqdim.add();

      client->text(FF("{\"success\":\"\"}"));
          
    }

    // ####################################
    // Light device full dimmer bind state
    // ####################################
    if (root.containsKey(FF("adimbind")) )
    {
      //root["adimbind"]
    }




    // ################################
    // Power device getter
    // ################################
    if (root.containsKey(FF("pdev")) )
    {
      String content = "", dirname = "";
       
      // list devices and send it
      // {"pdev":["filea.json","fileb.json"]}
       
      content += F("{\"pdev\":[\"\"");

      storage.pwdevice.getDir(dirname);
       
      // for each file in /sett/pdev directory do
      Dir directory = SPIFFS.openDir(dirname.c_str());
       
      while ( directory.next() )
      {
        content += F(", \"");
        content += directory.fileName();
        content += F("\" ");
      }

      content += F(" ]}");
   
      client->text(content);
    }

    // ################################
    // Power device setter
    // ################################
    if (root.containsKey(FF("pdevid")) )
    {
      // let's write the file configuration
      
      storage.pwdevice.save(root[F("pdevid")], data, len);
      
      client->text(FF("{\"success\":\"\"}"));
    }

    // ################################
    // Power device add setter
    // ################################
    if (root.containsKey(FF("pdevadd")) )
    {
      storage.pwdevice.add();

      client->text(FF("{\"success\":\"\"}"));
          
    }

    // ##################################
    // Power device remover
    // #################################
    if (root.containsKey(FF("pdevremove")) )
    {
      storage.pwdevice.remove(root[F("pdevremove")]);
     
      client->text(FF("{\"success\":\"\"}"));
    }



    // ################################
    // Update filesystem
    // ###############################
    if (root.containsKey(FF("updatefilesystem")) )
    {
      // let's write the file configuration
      
      storage.setUpdate(true);
      
      client->text(FF("{\"success\":\"\"}"));
    }
     
  }
  else if( strcmp((char*)data, FF("GET_OPAQ_WIFISETTINGS")) == 0 )
  {
    // let's send the wifisettings
    JsonObject& root = jsonBuffer.createObject();

    JsonObject& conf = root.createNestedObject("wifisettings");

    String wssid, wpwd, wclientssid, wclientpwd, wmode;
    storage.wifisett.getSSID(wssid);
    storage.wifisett.getPwd(wpwd);
    storage.wifisett.getClientSSID(wclientssid);
    storage.wifisett.getClientPwd(wclientpwd);
    storage.wifisett.getMode(wmode);
    
    conf[F("wssid")] = wssid.c_str();
    conf[F("wpwd")] = wpwd.c_str();
    conf[F("wchan")] = 6; // [TODO]
    
    conf[F("wssidsta")] = wclientssid.c_str();
    conf[F("wpwdsta")] = wclientpwd.c_str();
    conf[F("wmode")] = wmode.c_str();

    String tmp = F("");
    root.printTo(tmp);

    client->text(tmp);
  }
  else if( strcmp((char*)data, FF("GET_OPAQ_SUMMARY")) == 0 )
  {  
    String wssid;
    storage.wifisett.getSSID(wssid);
    
    JsonObject& root = jsonBuffer.createObject();
    root[F("version")] = OPAQ_VERSION;
    root[F("id")]      = ESP.getFlashChipId();
    root[F("status")]  = "Running without errors"; // [TODO]
    root[F("wstatus")] = "Radio is On"; // [TODO]
    root[F("wmode")]   = (storage.wifisett.getModeOperation())? "softAP" : "client";
    root[F("wssid")]   = wssid.c_str();
    root[F("wchan")]   = WiFi.channel();
    root[F("wdhcp")]   = "Enabled"; // [TODO]
    root[F("wmac")]    = WiFi.softAPmacAddress();
    root[F("wip")]     = (storage.wifisett.getModeOperation())? WiFi.softAPIP().toString() : WiFi.localIP().toString();

    String tmp = F("");
    root.printTo(tmp);

    client->text(tmp);
  }
  else
  {
    client->text(FF("{\"msg\":\"I got your text message\"}"));
  }
  
}

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){

  static String tmps = "";

  if(type == WS_EVT_CONNECT){
    //client connected
    DEBUG_MSG_OPAQWEBSOCKET(FF("ws[%s][%u] connect\n"), server->url(), client->id());
    //client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    //client disconnected
    DEBUG_MSG_OPAQWEBSOCKET(FF("ws[%s][%u] disconnect: %u\n"), server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    //error was received from the other end
    DEBUG_MSG_OPAQWEBSOCKET(FF("ws[%s][%u] error(%u): %s\n"), server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    //pong message was received (in response to a ping request maybe)
    DEBUG_MSG_OPAQWEBSOCKET(FF("ws[%s][%u] pong[%u]: %s\n"), server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    //data packet
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      DEBUG_MSG_OPAQWEBSOCKET(FF("ws[%s][%u] %s-message[%llu]: "), server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
      if(info->opcode == WS_TEXT){
        data[len] = 0;
        DEBUG_MSG_OPAQWEBSOCKET(FF("%s\n"), (char*)data);

        parseTextMessage(client, data, len);
        
      } else {
        for(size_t i=0; i < info->len; i++){
          DEBUG_MSG_OPAQWEBSOCKET(FF("%02x "), data[i]);
        }
        DEBUG_MSG_OPAQWEBSOCKET(FF("\n"));
      }

      
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          DEBUG_MSG_OPAQWEBSOCKET(FF("ws[%s][%u] %s-message start\n"), server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        DEBUG_MSG_OPAQWEBSOCKET(FF("ws[%s][%u] frame[%u] start[%llu]\n"), server->url(), client->id(), info->num, info->len);
      }

      DEBUG_MSG_OPAQWEBSOCKET(FF("ws[%s][%u] frame[%u] %s[%llu - %llu]: "), server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);
      if(info->message_opcode == WS_TEXT){
        data[len] = 0;
        DEBUG_MSG_OPAQWEBSOCKET(FF("%s\n"), (char*)data);

        tmps += (char*)data;
        
      } else {
        for(size_t i=0; i < len; i++){
          DEBUG_MSG_OPAQWEBSOCKET(FF("%02x "), data[i]);
        }
        DEBUG_MSG_OPAQWEBSOCKET(FF("\n"));
      }

      if((info->index + len) == info->len){
        DEBUG_MSG_OPAQWEBSOCKET(FF("ws[%s][%u] frame[%u] end[%llu]\n"), server->url(), client->id(), info->num, info->len);
        
        if(info->final){
          DEBUG_MSG_OPAQWEBSOCKET(FF("ws[%s][%u] %s-message end\n"), server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");

          parseTextMessage(client, (uint8_t*)const_cast<char*>(tmps.c_str()), tmps.length());

          tmps.~String();
          
        }
        
      }
    }
  }
}

/*
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

  String str;
  
  str += "{ ";
  for(int i=0; i<storage.getNumberOfPowerDevices(); i++)
  {
    uint8_t state = storage.getPDeviceState(i+1);
    
    str += "\"pw";
    str += String(i);
    str += "\": \"";
    str += (state == ON ) ? F("ON") :
      ( (state == OFF ) ? F("OFF") :
      ( (state == BINDING ) ? F("BINDING") :
      ( (state == UNBINDING ) ? F("UNBINDING") :
      ( (state == ON_PERMANENT ) ? F("PERMANENT ON") :
      ( (state == OFF_PERMANENT ) ? F("PERMANENT OFF") :
      (F(".")) ) ) ) ) );
    str += "\","; 
  }
  str = str.substring(0, str.lastIndexOf(","));
  str += "}";
  
  switch(type) {
      case WStype_DISCONNECTED:
          Serial.printf("[%u] Disconnected!\n", num);
          break;
      case WStype_CONNECTED:
          {
              IPAddress ip = webSocket.remoteIP(num);
              Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
          }
          break;
      case WStype_TEXT:
          Serial.printf("[%u] get Text: %s\n", num, payload);

          // echo data back to browser
          
          webSocket.sendTXT(num, str.c_str(), str.length());

          // send data to all connected clients
          //webSocket.broadcastTXT(payload, lenght);
          break;
      case WStype_BIN:
          Serial.printf("[%u] get binary lenght: %u\n", num, lenght);
          hexdump(payload, lenght);

          // echo data back to browser
          webSocket.sendBIN(num, payload, lenght);
          break;
  }

}*/
