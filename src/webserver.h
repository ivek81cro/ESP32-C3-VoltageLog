#ifndef WEBSERVER_H
#define WEBSERVER_H

// Definiraj HTTP konstante prije ESPAsyncWebServer
#define HTTP_GET     0x01
#define HTTP_HEAD    0x02
#define HTTP_POST    0x04
#define HTTP_PUT     0x08
#define HTTP_DELETE  0x10
#define HTTP_CONNECT 0x20
#define HTTP_OPTIONS 0x40
#define HTTP_TRACE   0x80
#define HTTP_PATCH   0x100
#define HTTP_ANY     0xFF

#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;

void setupWebServer();
void handleWebServer();

#endif
