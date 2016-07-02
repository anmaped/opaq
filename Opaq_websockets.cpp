
#include "Opaq_websockets.h"
#include "Opaq_storage.h"

WebSocketsServer webSocket = WebSocketsServer(81);


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

}
