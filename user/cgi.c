﻿/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <esp8266.h>
#include "cgi.h"
#include "rboot-ota.h"
#include "timer.h"
#include "stdout.h"
#include "control.h"

//WiFi access point data
typedef struct
{
    char ssid[32];
    char bssid[8];
    int channel;
    char rssi;
    char enc;
} ApData;

//Scan result
typedef struct
{
    char scanInProgress; //if 1, don't access the underlying stuff from the webpage.
    ApData **apData;
    int noAps;
} ScanResultData;

//Static scan status storage.
static ScanResultData cgiWifiAps;

#define CONNTRY_IDLE 0
#define CONNTRY_WORKING 1
#define CONNTRY_SUCCESS 2
#define CONNTRY_FAIL 3
//Connection result var
static int connTryStatus = CONNTRY_IDLE;
static os_timer_t resetTimer;
static os_timer_t ScanTimeoutTimer;

// CGI code for stream control of index page
// This function is called every time a POST action is received for stream.cgi
int ICACHE_FLASH_ATTR IndexStreamCgi(HttpdConnData *connData)
{
    int len;
    char buff[1024];
    uint32 select = 0;



    len = httpdFindArg(connData->post->buff, "stream", buff, sizeof(buff));
    if (len != 0)
    {
        select = strtol(buff, NULL, 0);
    }

    len = httpdFindArg(connData->post->buff, "stream_control", buff, sizeof(buff));
    if (len != 0)
    {
        if (os_strcmp(buff, "PLAY") == 0)
        {
            HTTPC_vStartStreaming(&stream_address[select][0], "Icy-MetaData:1\r\n", Timer_StreamingCallback);
        }
        if (os_strcmp(buff, "STOP") == 0)
            HTTPC_vStopStreaming();
    }

    if (connData->conn == NULL)
    {
        //Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    httpdRedirect(connData, "index.html");
    return HTTPD_CGI_DONE;
}

// CGI code for playback control of index page
// This function is called every time a POST action is received for playback.cgi
int ICACHE_FLASH_ATTR IndexPlaybackCgi(HttpdConnData *connData)
{
    int len;
    char buff[1024];

    len = httpdFindArg(connData->post->buff, "autoplay", buff, sizeof(buff));
    if (len != 0)
    {
        Control_vSetAutoStart(strtol(buff, NULL, 0));
    }

    if (connData->conn == NULL)
    {
        //Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    httpdRedirect(connData, "index.html");
    return HTTPD_CGI_DONE;
}

// CGI code for volume control of index page
int ICACHE_FLASH_ATTR IndexVolumeCgi(HttpdConnData *connData)
{
    int len;
    char buff[1024];

    if (connData->conn == NULL)
    {
        //Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    len = httpdFindArg(connData->post->buff, "volume", buff, sizeof(buff));
    if (len != 0)
        Control_vSetVolume(strtol(buff, NULL, 0));

    httpdRedirect(connData, "index.tpl");
    return HTTPD_CGI_DONE;
}

// CGI code for enhancer control of index page
int ICACHE_FLASH_ATTR IndexEnhancerCgi(HttpdConnData *connData)
{
    int len;
    char buff[1024];
    Control_tstEnhancerSettings data_pointer;

    if (connData->conn == NULL)
    {
        //Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    // Get current settings
    Control_vGetEnhancer(&data_pointer);

    len = httpdFindArg(connData->post->buff, "treble_amp", buff, sizeof(buff));
    if (len != 0)
        data_pointer.TrebleAmp = strtol(buff, NULL, 0);

    len = httpdFindArg(connData->post->buff, "treble_lim", buff, sizeof(buff));
    if (len != 0)
        data_pointer.TrebleLim = strtol(buff, NULL, 0);

    len = httpdFindArg(connData->post->buff, "bass_amp", buff, sizeof(buff));
    if (len != 0)
        data_pointer.BassAmp = strtol(buff, NULL, 0);

    len = httpdFindArg(connData->post->buff, "bass_lim", buff, sizeof(buff));
    if (len != 0)
        data_pointer.BassLim = strtol(buff, NULL, 0);

    // Set new settings
    Control_vSetEnhancer(&data_pointer);

    httpdRedirect(connData, "index.html");
    return HTTPD_CGI_DONE;
}

// CGI code for spartial level control of index page
int ICACHE_FLASH_ATTR IndexSpartialCgi(HttpdConnData *connData)
{
    int len;
    char buff[1024];
    Control_tstEnhancerSettings data_pointer;

    if (connData->conn == NULL)
    {
        //Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    // Get current settings
    len=httpdFindArg(connData->post->buff, "SpartialProcessingLevel", buff, sizeof(buff));
    if (len!=0)
    {
        uint8 temp = strtol(buff, NULL, 0);
        Control_vSetSpartialProcessingLevel(temp);
        VS1053_vSetSpartialProcessing(temp);
    }

    httpdRedirect(connData, "index.html");
    return HTTPD_CGI_DONE;
}

// Default template code for the index page
// When index.html is initially loaded (or refreshed) this function is called to
// replace all %sometemplate% macro fields
int ICACHE_FLASH_ATTR IndexTemplate(HttpdConnData *connData, char *token, void **arg)
{
    char buf[100];
    Control_tstEnhancerSettings data_pointer;

    if (token == NULL)
        return HTTPD_CGI_DONE;

    // Macro "FIRMW_VERSION" is defined in makefile
    if (os_strcmp(token, "Version")==0)
        httpdSend(connData, FIRMW_VERSION, -1);

    // Macro "DEBUG_VERSION" is defined in makefile
#ifdef DEBUG_VERSION
    if (os_strcmp(token, "Debug")==0)
        httpdSend(connData, " Debug", -1);
#endif

    if (os_strcmp(token, "autoplay")==0)
        if(Control_u8GetAutoStart() == 1)
            httpdSend(connData, "checked='checked'", -1);

    if (os_strcmp(token, "volume")==0)
    {
        os_sprintf(buf, "%d", Control_u8GetVolume());
        httpdSend(connData, buf, -1);
    }

    if (os_strcmp(token, "treble_amp")==0)
    {
        Control_vGetEnhancer(&data_pointer);
        os_sprintf(buf, "%d", data_pointer.TrebleAmp);
        httpdSend(connData, buf, -1);
    }

    if (os_strcmp(token, "treble_lim")==0)
    {
        Control_vGetEnhancer(&data_pointer);
        os_sprintf(buf, "%d", data_pointer.TrebleLim);
        httpdSend(connData, buf, -1);
    }

    if (os_strcmp(token, "bass_amp")==0)
    {
        Control_vGetEnhancer(&data_pointer);
        os_sprintf(buf, "%d", data_pointer.BassAmp);
        httpdSend(connData, buf, -1);
    }

    if (os_strcmp(token, "bass_lim")==0)
    {
        Control_vGetEnhancer(&data_pointer);
        os_sprintf(buf, "%d", data_pointer.BassLim);
        httpdSend(connData, buf, -1);
    }

    if (os_strcmp(token, "SpartialProcessingLevel_Off")==0)
        if(Control_u8GetSpartialProcessingLevel() == Control_enSpartialProcessing_Off)
            httpdSend(connData, "checked='checked'", -1);

    if (os_strcmp(token, "SpartialProcessingLevel_Minimal")==0)
        if(Control_u8GetSpartialProcessingLevel() == Control_enSpartialProcessing_Minimal)
            httpdSend(connData, "checked='checked'", -1);

    if (os_strcmp(token, "SpartialProcessingLevel_Normal")==0)
        if(Control_u8GetSpartialProcessingLevel() == Control_enSpartialProcessing_Normal)
            httpdSend(connData, "checked='checked'", -1);

    if (os_strcmp(token, "SpartialProcessingLevel_Extreme")==0)
        if(Control_u8GetSpartialProcessingLevel() == Control_enSpartialProcessing_Extreme)
            httpdSend(connData, "checked='checked'", -1);

    if (os_strcmp(token, "StreamSelected_0")==0)
        //if(Control_u8GetSpartialProcessingLevel() == Control_enSpartialProcessing_Extreme)
        httpdSend(connData, "checked='checked'", -1);

    if (os_strcmp(token, "StreamName_0")==0)
        httpdSend(connData, &stream_name[0][0], -1);
    if (os_strcmp(token, "StreamName_1")==0)
        httpdSend(connData, &stream_name[1][0], -1);
    if (os_strcmp(token, "StreamName_2")==0)
        httpdSend(connData, &stream_name[2][0], -1);

    return HTTPD_CGI_DONE;
}

//Cgi that turns the LED on or off according to the 'led' param in the POST data
int ICACHE_FLASH_ATTR cgiSettings(HttpdConnData *connData)
{
    int len;
    char buff[1024];

    if (connData->conn == NULL)
    {
        //Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    httpdRedirect(connData, "settings.tpl");
    return HTTPD_CGI_DONE;
}

//Template code for the settings page.
int ICACHE_FLASH_ATTR tplSettings(HttpdConnData *connData, char *token, void **arg)
{
    return HTTPD_CGI_DONE;
}

//Callback the code calls when a wlan ap scan is done. Basically stores the result in
//the cgiWifiAps struct.
void ICACHE_FLASH_ATTR wifiScanDoneCb(void *arg, STATUS status)
{
    int n;
    struct bss_info *bss_link = (struct bss_info*) arg;
    httpd_printf("wifiScanDoneCb %d\n", status);
    if (status != OK)
    {
        cgiWifiAps.scanInProgress = 0;
        return;
    }

    //Clear prev ap data if needed.
    if (cgiWifiAps.apData != NULL)
    {
        for (n = 0; n < cgiWifiAps.noAps; n++)
            free(cgiWifiAps.apData[n]);
        free(cgiWifiAps.apData);
    }

    //Count amount of access points found.
    n = 0;
    while (bss_link != NULL)
    {
        bss_link = bss_link->next.stqe_next;
        n++;
    }
    //Allocate memory for access point data
    cgiWifiAps.apData = (ApData**) malloc(sizeof(ApData*) * n);
    if (cgiWifiAps.apData == NULL)
    {
        printf("Out of memory allocating apData\n");
        return;
    }
    cgiWifiAps.noAps = n;
    httpd_printf("Scan done: found %d APs\n", n);

    //Copy access point data to the static struct
    n = 0;
    bss_link = (struct bss_info*) arg;
    while (bss_link != NULL)
    {
        if (n >= cgiWifiAps.noAps)
        {
            //This means the bss_link changed under our nose. Shouldn't happen!
            //Break because otherwise we will write in unallocated memory.
            httpd_printf("Huh? I have more than the allocated %d aps!\n", cgiWifiAps.noAps);
            break;
        }
        //Save the ap data.
        cgiWifiAps.apData[n] = (ApData*) malloc(sizeof(ApData));
        if (cgiWifiAps.apData[n] == NULL)
        {
            httpd_printf("Can't allocate mem for ap buff.\n");
            cgiWifiAps.scanInProgress = 0;
            return;
        }
        cgiWifiAps.apData[n]->rssi = bss_link->rssi;
        cgiWifiAps.apData[n]->channel = bss_link->channel;
        cgiWifiAps.apData[n]->enc = bss_link->authmode;
        strncpy(cgiWifiAps.apData[n]->ssid, (char* )bss_link->ssid, 32);
        strncpy(cgiWifiAps.apData[n]->bssid, (char* )bss_link->bssid, 6);

        bss_link = bss_link->next.stqe_next;
        n++;
    }
    //We're done.
    cgiWifiAps.scanInProgress = 0;
}

static void ICACHE_FLASH_ATTR ScanTimoutTimerCb(void *arg)
{
    struct ip_info ipinfo;
    struct station_config staConfig;
    wifi_station_get_config(&staConfig);
    // Station name set or not
    if (*staConfig.ssid == 0)
    {
        wifi_set_opmode(SOFTAP_MODE);

        wifi_softap_dhcps_stop();
        IP4_ADDR(&ipinfo.ip, 192, 168, 1, 1);
        IP4_ADDR(&ipinfo.gw, 192, 168, 1, 1);
        IP4_ADDR(&ipinfo.netmask, 255, 255, 255, 0);
        wifi_set_ip_info(SOFTAP_IF, &ipinfo);
        wifi_softap_dhcps_start();
    }
    else
    {
        wifi_set_opmode(STATION_MODE);
    }
}

//Routine to start a WiFi access point scan.
static void ICACHE_FLASH_ATTR wifiStartScan()
{
//	int x;
    if (cgiWifiAps.scanInProgress)
        return;
    cgiWifiAps.scanInProgress = 1;
    wifi_set_opmode(STATIONAP_MODE);
    wifi_station_scan(NULL, wifiScanDoneCb);

    //Schedule disconnect/connect
    os_timer_disarm(&ScanTimeoutTimer);
    os_timer_setfn(&ScanTimeoutTimer, ScanTimoutTimerCb, NULL);
    os_timer_arm(&ScanTimeoutTimer, 30000, 0); //time out after 15 secs of trying to connect
}

//This CGI is called from the bit of AJAX-code in wifi.tpl. It will initiate a
//scan for access points and if available will return the result of an earlier scan.
//The result is embedded in a bit of JSON parsed by the javascript in wifi.tpl.
int ICACHE_FLASH_ATTR cgiWiFiScan(HttpdConnData *connData)
{
    int pos = (int) connData->cgiData;
    int len;
    char buff[1024];

    if (!cgiWifiAps.scanInProgress && pos != 0)
    {
        //Fill in json code for an access point
        if (pos - 1 < cgiWifiAps.noAps)
        {
            len = sprintf(buff, "{\"essid\": \"%s\", \"bssid\": \"" MACSTR "\", \"rssi\": \"%d\", \"enc\": \"%d\", \"channel\": \"%d\"}%s\n", cgiWifiAps.apData[pos - 1]->ssid, MAC2STR(cgiWifiAps.apData[pos-1]->bssid), cgiWifiAps.apData[pos - 1]->rssi, cgiWifiAps.apData[pos - 1]->enc, cgiWifiAps.apData[pos - 1]->channel,
                    (pos - 1 == cgiWifiAps.noAps - 1) ? "" : ",");
            httpdSend(connData, buff, len);
        }
        pos++;
        if ((pos - 1) >= cgiWifiAps.noAps)
        {
            len = sprintf(buff, "]\n}\n}\n");
            httpdSend(connData, buff, len);
            //Also start a new scan.
            wifiStartScan();
            return HTTPD_CGI_DONE;
        }
        else
        {
            connData->cgiData = (void*) pos;
            return HTTPD_CGI_MORE;
        }
    }

    httpdStartResponse(connData, 200);
    httpdHeader(connData, "Content-Type", "text/json");
    httpdEndHeaders(connData);

    if (cgiWifiAps.scanInProgress == 1)
    {
        //We're still scanning. Tell Javascript code that.
        len = sprintf(buff, "{\n \"result\": { \n\"inProgress\": \"1\"\n }\n}\n");
        httpdSend(connData, buff, len);
        return HTTPD_CGI_DONE;
    }
    else
    {
        //We have a scan result. Pass it on.
        len = sprintf(buff, "{\n \"result\": { \n\"inProgress\": \"0\", \n\"APs\": [\n");
        httpdSend(connData, buff, len);
        if (cgiWifiAps.apData == NULL)
            cgiWifiAps.noAps = 0;
        connData->cgiData = (void*) 1;
        return HTTPD_CGI_MORE;
    }
}

//Temp store for new ap info.
static struct station_config stconf;

//This routine is ran some time after a connection attempt to an access point. If
//the connect succeeds, this gets the module in STA-only mode.
static void ICACHE_FLASH_ATTR resetTimerCb(void *arg)
{
    int x = wifi_station_get_connect_status();
    if (x == STATION_GOT_IP)
    {
        //Go to STA mode. This needs a reset, so do that.
        httpd_printf("Got IP. Going into STA mode..\n");
        wifi_set_opmode(STATION_MODE);
        //Schedule disconnect/connect
        os_timer_disarm(&ScanTimeoutTimer);
        // Restart no longer needed after firmware v9.something
        //system_restart();
    }
    else
    {
        connTryStatus = CONNTRY_FAIL;
        httpd_printf("Connect fail. Not going into STA-only mode.\n");
        //Maybe also pass this through on the webpage?
    }
}

//Actually connect to a station. This routine is timed because I had problems
//with immediate connections earlier. It probably was something else that caused it,
//but I can't be arsed to put the code back :P
static void ICACHE_FLASH_ATTR reassTimerCb(void *arg)
{
    int x;
    httpd_printf("Try to connect to AP....\n");
    wifi_station_disconnect();
    wifi_station_set_config(&stconf);
    wifi_station_connect();
    x = wifi_get_opmode();
    connTryStatus = CONNTRY_WORKING;
    if (x != 1)
    {
        //Schedule disconnect/connect
        os_timer_disarm(&resetTimer);
        os_timer_setfn(&resetTimer, resetTimerCb, NULL);
        os_timer_arm(&resetTimer, 15000, 0); //time out after 15 secs of trying to connect
    }
}

//This cgi uses the routines above to connect to a specific access point with the
//given ESSID using the given password.
int ICACHE_FLASH_ATTR cgiWiFiConnect(HttpdConnData *connData)
{
    char essid[128];
    char passwd[128];
    static os_timer_t reassTimer;

    if (connData->conn == NULL)
    {
        //Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    httpdFindArg(connData->post->buff, "essid", essid, sizeof(essid));
    httpdFindArg(connData->post->buff, "passwd", passwd, sizeof(passwd));

    if (essid[0] == '$')
    {
        // hidden network
        httpdFindArg(connData->post->buff, "essid_hidden", essid, sizeof(essid));
    }

    strncpy((char* )stconf.ssid, essid, 32);
    strncpy((char* )stconf.password, passwd, 64);
    httpd_printf("Try to connect to AP %s pw %s\n", essid, passwd);

    //Schedule disconnect/connect
    os_timer_disarm(&reassTimer);
    os_timer_setfn(&reassTimer, reassTimerCb, NULL);
//Set to 0 if you want to disable the actual reconnecting bit
#ifdef DEMO_MODE
	httpdRedirect(connData, "/wifi");
#else
    os_timer_arm(&reassTimer, 500, 0);
    httpdRedirect(connData, "connecting.html");
#endif
    return HTTPD_CGI_DONE;
}

//This cgi uses the routines above to connect to a specific access point with the
//given ESSID using the given password.
int ICACHE_FLASH_ATTR cgiWiFiSetMode(HttpdConnData *connData)
{
    int len;
    char buff[1024];

    if (connData->conn == NULL)
    {
        //Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    len = httpdFindArg(connData->getArgs, "mode", buff, sizeof(buff));
    if (len != 0)
    {
        httpd_printf("cgiWifiSetMode: %s\n", buff);
#ifndef DEMO_MODE
        wifi_set_opmode(strtol(buff, NULL, 0));
        system_restart();
#endif
    }
    httpdRedirect(connData, "/wifi");
    return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR cgiWiFiConnStatus(HttpdConnData *connData)
{
    char buff[1024];
    int len;
    struct ip_info info;
    int st = wifi_station_get_connect_status();
    httpdStartResponse(connData, 200);
    httpdHeader(connData, "Content-Type", "text/json");
    httpdEndHeaders(connData);
    if (connTryStatus == CONNTRY_IDLE)
    {
        len = sprintf(buff, "{\n \"status\": \"idle\"\n }\n");
    }
    else
        if (connTryStatus == CONNTRY_WORKING || connTryStatus == CONNTRY_SUCCESS)
        {
            if (st == STATION_GOT_IP)
            {
                wifi_get_ip_info(0, &info);
                len = sprintf(buff, "{\n \"status\": \"success\",\n \"ip\": \"%d.%d.%d.%d\" }\n", (info.ip.addr >> 0) & 0xff, (info.ip.addr >> 8) & 0xff, (info.ip.addr >> 16) & 0xff, (info.ip.addr >> 24) & 0xff);
                //Reset into AP-only mode sooner.
                os_timer_disarm(&resetTimer);
                os_timer_setfn(&resetTimer, resetTimerCb, NULL);
                os_timer_arm(&resetTimer, 1000, 0);
            }
            else
            {
                len = sprintf(buff, "{\n \"status\": \"working\"\n }\n");
            }
        }
        else
        {
            len = sprintf(buff, "{\n \"status\": \"fail\"\n }\n");
        }

    httpdSend(connData, buff, len);
    return HTTPD_CGI_DONE;
}

//Template code for the WLAN page.
int ICACHE_FLASH_ATTR tplWlan(HttpdConnData *connData, char *token, void **arg)
{
    char buff[1024];
    int x;
    uint8 status;
    static struct station_config stconf;
    struct ip_info info;
    uint8 mac[6];

    if (token == NULL)
        return HTTPD_CGI_DONE;

    strcpy(buff, "Unknown");
    if (strcmp(token, "Wifi_Mode") == 0)
    {
        x = wifi_get_opmode();
        if (x == 1)
            strcpy(buff, "Client");
        if (x == 2)
            strcpy(buff, "AccessPoint");
        if (x == 3)
            strcpy(buff, "Client+AccessPoint");
    }
    else
        if (strcmp(token, "Wifi_SSID") == 0)
        {
            wifi_station_get_config(&stconf);
            strcpy(buff, (char* )stconf.ssid);
        }
        else
            if (strcmp(token, "Wifi_STATUS") == 0)
            {
                status = wifi_station_get_connect_status();
                if (status == STATION_IDLE)
                    strcpy(buff, "Basisstation nicht ausgewählt");
                if (status == STATION_CONNECTING)
                    strcpy(buff, "Verbindung wird aufgebaut");
                if (status == STATION_WRONG_PASSWORD)
                    strcpy(buff, "Passwort inkorrekt");
                if (status == STATION_NO_AP_FOUND)
                    strcpy(buff, "Basisstation nicht gefunden");
                if (status == STATION_CONNECT_FAIL)
                    strcpy(buff, "Verbindung fehlgeschlagen");
                if (status == STATION_GOT_IP)
                    strcpy(buff, "Verbunden");
            }
            else
                if (strcmp(token, "Wifi_IP") == 0)
                {
                    status = wifi_station_get_connect_status();
                    if (status == STATION_GOT_IP)
                    {
                        wifi_get_ip_info(0, &info);
                        sprintf(buff, "%d.%d.%d.%d", (info.ip.addr >> 0) & 0xff, (info.ip.addr >> 8) & 0xff, (info.ip.addr >> 16) & 0xff, (info.ip.addr >> 24) & 0xff);
                    }
                    else
                    {
                        strcpy(buff, "---");
                    }
                }
                else
                    if (strcmp(token, "Wifi_MAC") == 0)
                    {
                        wifi_get_macaddr(STATION_IF, mac);
                        sprintf(buff, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                    }

    httpdSend(connData, buff, -1);
    return HTTPD_CGI_DONE;
}

static struct espconn *conn;
ip_addr_t ip;
static os_timer_t ota_prepare_timer;

static const char* ICACHE_FLASH_ATTR esp_errstr(sint8 err)
{
    switch (err)
    {
        case ESPCONN_OK:
            return "No error, everything OK.";
        case ESPCONN_MEM:
            return "Out of memory error.";
        case ESPCONN_TIMEOUT:
            return "Timeout.";
        case ESPCONN_RTE:
            return "Routing problem.";
        case ESPCONN_INPROGRESS:
            return "Operation in progress.";
        case ESPCONN_ABRT:
            return "Connection aborted.";
        case ESPCONN_RST:
            return "Connection reset.";
        case ESPCONN_CLSD:
            return "Connection closed.";
        case ESPCONN_CONN:
            return "Not connected.";
        case ESPCONN_ARG:
            return "Illegal argument.";
        case ESPCONN_ISCONN:
            return "Already connected.";
    }
}

// clean up at the end of the update
// will call the user call back to indicate completion
void ICACHE_FLASH_ATTR cgiOtaDeinit()
{
    os_timer_disarm(&ota_prepare_timer);

    // if connected, disconnect and clean up connection
    if (conn)
        espconn_disconnect(conn);
}

// called when connection receives data (hopefully the rom)
static void ICACHE_FLASH_ATTR cgiOtaReceiveCb(void *arg, char *pusrdata, unsigned short length)
{

    char *ptrData, *ptrLen, *ptr;

    // disarm the timer
    os_timer_disarm(&ota_prepare_timer);

    // valid http response?
    if ((ptrLen = (char*) os_strstr(pusrdata, "Content-Length: ")) && (ptrData = (char*) os_strstr(ptrLen, "\r\n\r\n")) && (os_strncmp(pusrdata + 9, "200", 3) == 0))
    {

        // end of header/start of data
        ptrData += 4;
        // length of data after header in this chunk
        length -= (ptrData - pusrdata);
        // process current chunk
        myprintf("OTA: Version found = %s\n", (uint8*) ptrData);

        // fail, not a valid http header/non-200 response/etc.
        cgiOtaDeinit();
    }
}

// disconnect callback, clean up the connection
// we also call this ourselves
static void ICACHE_FLASH_ATTR cgiOtaDisconnectCb(void *arg)
{
    // use passed ptr, as upgrade struct may have gone by now
    struct espconn *Conn = (struct espconn*) arg;

    os_timer_disarm(&ota_prepare_timer);

    if (Conn)
    {
        if (Conn->proto.tcp)
        {
            os_free(Conn->proto.tcp);
        }
        os_free(Conn);
    }
}

// successfully connected to update server, send the request
static void ICACHE_FLASH_ATTR cgiOtaConnectCb(void *arg)
{

    uint8 *request;

    // disable the timeout
    os_timer_disarm(&ota_prepare_timer);

    // register connection callbacks
    espconn_regist_disconcb(conn, cgiOtaDisconnectCb);
    espconn_regist_recvcb(conn, cgiOtaReceiveCb);

    // http request string
    request = (uint8*) os_malloc(512);
    if (!request)
    {
        myprintf("OTA: No ram!\n");
        cgiOtaDeinit();
        return;
    }
    os_sprintf((char*) request, "GET /wordclock_update/010_Version.txt HTTP/1.0\r\nHost: " OTA_HOST "\r\n" HTTP_HEADER);

    // send the http request, with timeout for reply
    os_timer_setfn(&ota_prepare_timer, (os_timer_func_t*) cgiOtaDeinit, 0);
    os_timer_arm(&ota_prepare_timer, 10000, 0);
    espconn_sent(conn, request, os_strlen((char*) request));
    os_free(request);
}

// call back for lost connection
static void ICACHE_FLASH_ATTR cgiOtaReconnectCb(void *arg, sint8 errType)
{
    myprintf("OTA: Connection error ");
    myprintf(esp_errstr(errType));
    myprintf("\n");
    // not connected so don't call disconnect on the connection
    // but call our own disconnect callback to do the cleanup
    cgiOtaDisconnectCb(conn);
}

// connection attempt timed out
static void ICACHE_FLASH_ATTR cgiOtaConnectTimeoutCb()
{
    myprintf("OTA: Connect timeout.\n");
    // not connected so don't call disconnect on the connection
    // but call our own disconnect callback to do the cleanup
    cgiOtaDisconnectCb(conn);
}

void ICACHE_FLASH_ATTR cgiOtaDnsIpFound(const char *name, ip_addr_t *ipaddr, void *arg)
{
    if (ipaddr == 0)
    {
        myprintf("OTA: DNS lookup failed for  ");
        myprintf(OTA_HOST);
        myprintf("\n");
        // not connected so don't call disconnect on the connection
        // but call our own disconnect callback to do the cleanup
        cgiOtaDisconnectCb(conn);
        return;
    }

    // set up connection
    conn->type = ESPCONN_TCP;
    conn->state = ESPCONN_NONE;
    conn->proto.tcp->local_port = espconn_port();
    conn->proto.tcp->remote_port = OTA_PORT;
    *(ip_addr_t*) conn->proto.tcp->remote_ip = *ipaddr;
    // set connection call backs
    espconn_regist_connectcb(conn, cgiOtaConnectCb);
    espconn_regist_reconcb(conn, cgiOtaReconnectCb);

    // try to connect
    espconn_connect(conn);

    // set connection timeout timer
    os_timer_disarm(&ota_prepare_timer);
    os_timer_setfn(&ota_prepare_timer, (os_timer_func_t*) cgiOtaConnectTimeoutCb, 0);
    os_timer_arm(&ota_prepare_timer, 10000, 0);
}

void ICACHE_FLASH_ATTR cgiPrepareOtaUpdate(void)
{
    err_t result;
    conn = (struct espconn*) os_zalloc(sizeof(struct espconn));

    if (!conn)
    {
        myprintf("No ram!\n");
        return;
    }
    // set up the udp "connection"
    conn->proto.tcp = (esp_tcp*) os_zalloc(sizeof(esp_tcp));
    if (!conn->proto.tcp)
    {
        myprintf("No ram!\n");
        os_free(conn);
        return;
    }

    // dns lookup
    result = espconn_gethostbyname(conn, OTA_HOST, &ip, cgiOtaDnsIpFound);

    if (result == ESPCONN_OK)
    {
        // hostname is already cached or is actually a dotted decimal ip address
        cgiOtaDnsIpFound(0, &ip, conn);
    }
    else
        if (result == ESPCONN_INPROGRESS)
        {
            // lookup taking place, will call upgrade_resolved on completion
        }
        else
        {
            myprintf("OTA: DNS error!\n");
            os_free(conn->proto.tcp);
            os_free(conn);
            return;
        }
}

// Handle request to update
int ICACHE_FLASH_ATTR cgiUpdateFirmware(HttpdConnData *connData)
{
    if (connData->conn == NULL)
    {
        //Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    cgiPrepareOtaUpdate();

    httpdStartResponse(connData, 200);
    httpdHeader(connData, "Content-Type", "text/plain");
    httpdEndHeaders(connData);
    httpdSend(connData, "Upgrading...", -1);
    return HTTPD_CGI_DONE;
}

//Broadcast the Debug Output over connected websockets
void ICACHE_FLASH_ATTR websockDebugOut(char *buff)
{
    cgiWebsockBroadcast("/websocket/ws.cgi", buff, os_strlen(buff), WEBSOCK_FLAG_NONE);
}

//On reception of a message, send "You sent: " plus whatever the other side sent
void ICACHE_FLASH_ATTR myWebsocketRecv(Websock *ws, char *data, int len, int flags)
{
    int i;
    char buff[128];
    os_sprintf(buff, "You sent: ");
    for (i = 0; i < len; i++)
        buff[i + 10] = data[i];
    buff[i + 10] = 0;
    cgiWebsocketSend(ws, buff, os_strlen(buff), WEBSOCK_FLAG_NONE);
}

//Websocket connected. Install reception handler and send welcome message.
void ICACHE_FLASH_ATTR myWebsocketConnect(Websock *ws)
{
    ws->recvCb = myWebsocketRecv;
    // Here we are able to send some data to our new connected socket
    //cgiWebsocketSend(ws, "Hi, Websocket!", 14, WEBSOCK_FLAG_NONE);
}

//On reception of a message, echo it back verbatim
void ICACHE_FLASH_ATTR myEchoWebsocketRecv(Websock *ws, char *data, int len, int flags)
{
    os_printf("EchoWs: echo, len=%d\n", len);
    cgiWebsocketSend(ws, data, len, flags);
}

//Echo websocket connected. Install reception handler.
void ICACHE_FLASH_ATTR myEchoWebsocketConnect(Websock *ws)
{
    os_printf("EchoWs: connect\n");
    ws->recvCb = myEchoWebsocketRecv;
}
