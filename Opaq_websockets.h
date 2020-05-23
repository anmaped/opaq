
#include "version.h"
#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"

#define DEBUG_MSG_OPAQWEBSOCKET(...)

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len);
