#ifndef CGI_H
#define CGI_H

#include "httpd.h"
#include "cgiwebsocket.h"

int ICACHE_FLASH_ATTR IndexStreamCgi(HttpdConnData *connData);
int ICACHE_FLASH_ATTR IndexPlaybackCgi(HttpdConnData *connData);
int ICACHE_FLASH_ATTR IndexVolumeCgi(HttpdConnData *connData);
int ICACHE_FLASH_ATTR IndexEnhancerCgi(HttpdConnData *connData);
int ICACHE_FLASH_ATTR IndexSpartialCgi(HttpdConnData *connData);
int ICACHE_FLASH_ATTR IndexTemplate(HttpdConnData *connData, char *token, void **arg);
int ICACHE_FLASH_ATTR cgiSettings(HttpdConnData *connData);
int tplSettings(HttpdConnData *connData, char *token, void **arg);
int cgiWiFiScan(HttpdConnData *connData);
int tplWlan(HttpdConnData *connData, char *token, void **arg);
int cgiUpdateFirmware(HttpdConnData *connData);
int cgiWiFi(HttpdConnData *connData);
int cgiWiFiConnect(HttpdConnData *connData);
int cgiWiFiSetMode(HttpdConnData *connData);
int cgiWiFiConnStatus(HttpdConnData *connData);

//Broadcast the Debug Output over connected websockets
void ICACHE_FLASH_ATTR websockDebugOut(char* buff);
//On reception of a message, send "You sent: " plus whatever the other side sent
void ICACHE_FLASH_ATTR myWebsocketRecv(Websock *ws, char *data, int len, int flags);
//Websocket connected. Install reception handler and send welcome message.
void ICACHE_FLASH_ATTR myWebsocketConnect(Websock *ws);
//On reception of a message, echo it back verbatim
void ICACHE_FLASH_ATTR myEchoWebsocketRecv(Websock *ws, char *data, int len, int flags);
//Echo websocket connected. Install reception handler.
void ICACHE_FLASH_ATTR myEchoWebsocketConnect(Websock *ws);

#endif
