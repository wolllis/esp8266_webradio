/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Martin d'Allens <martin.dallens@gmail.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

typedef void (*http_callback)(char *response_body, int http_status, char *full_response);

void ICACHE_FLASH_ATTR HTTPC_vStartStreaming(const char * url, const char * headers, http_callback user_callback);
void ICACHE_FLASH_ATTR HTTPC_vStopStreaming(void);

#endif
