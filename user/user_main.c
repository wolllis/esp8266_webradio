/* MIT License
 *
 * Copyright (c) 2019, wolllis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <esp8266.h>
#include "uart.h"
#include "httpd.h"
#include "httpdespfs.h"
#include "cgi.h"
#include "cgiflash.h"
#include "stdout.h"
#include "auth.h"
#include "espfs.h"
#include "webpages-espfs.h"
#include "cgiwebsocket.h"
#include "timer.h"
#include "sha256.h"
#include "vs1053.h"
#include "httpclient.h"

unsigned char hash[32];

void user_pre_init()
{
    //Not needed, but some SDK versions want this defined.
}

uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map)
    {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 8;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

//Function that tells the authentication system what users/passwords live on the system.
//This is disabled in the default build; if you want to try it, enable the authBasic line in
//the builtInUrls below.
int ICACHE_FLASH_ATTR myPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen)
{
    char buff[12];
    if (no == 0)
    {
        os_strcpy(user, "admin");
        sprintf(buff, "%02x%02x%02x%02x", hash[4], hash[5], hash[6], hash[7]);
        os_memcpy(pass, buff, 9);
        return 1;
        //Add more users this way. Check against incrementing no for each user added.
        //	} else if (no==1) {
        //		os_strcpy(user, "user1");
        //		os_strcpy(pass, "something");
        //		return 1;
    }
    return 0;
}

/*
 This is the main url->function dispatching data struct.
 In short, it's a struct with various URLs plus their handlers. The handlers can
 be 'standard' CGI functions you wrote, or 'special' CGIs requiring an argument.
 They can also be auth-functions. An asterisk will match any url starting with
 everything before the asterisks; "*" matches everything. The list will be
 handled top-down, so make sure to put more specific rules above the more
 general ones. Authorization things (like authBasic) act as a 'barrier' and
 should be placed above the URLs they protect.
 */
HttpdBuiltInUrl builtInUrls[] = {
        { "*", cgiRedirectApClientToHostname, "webradio." },
        { "/", cgiRedirect, "/index.html" },
        { "/index.html", cgiEspFsTemplate, IndexTemplate },
        { "/stream.cgi", IndexStreamCgi, NULL },
        { "/playback.cgi", IndexPlaybackCgi, NULL },
        { "/volume.cgi", IndexVolumeCgi, NULL },
        { "/enhancer.cgi", IndexEnhancerCgi, NULL },
        { "/spartial.cgi", IndexSpartialCgi, NULL },
        { "/wifi/*", authBasic, myPassFn },
        { "/wifi", cgiRedirect, "/wifi/wifi.html" },
        { "/wifi/", cgiRedirect, "/wifi/wifi.html" },
        { "/wifi/wifiscan.cgi", cgiWiFiScan, NULL },
        { "/wifi/wifi.html", cgiEspFsTemplate, tplWlan },
        { "/wifi/connect.cgi", cgiWiFiConnect, NULL },
        { "/wifi/connstatus.cgi", cgiWiFiConnStatus, NULL },
        { "*", cgiEspFsHook, NULL }, //Catch-all cgi function for the filesystem
        { NULL, NULL, NULL } };

void wifi_handle_event_cb(System_Event_t *evt)
{
    struct ip_info ipinfo;
    static uint8 u8DisconnectCounter = 0;

    //myprintf("event %x\n", evt->event);
    switch (evt->event)
    {
        case EVENT_STAMODE_CONNECTED:
            myprintf("connect to ssid %s, channel %d\n", evt->event_info.connected.ssid, evt->event_info.connected.channel);
            u8DisconnectCounter = 0;
            break;

        case EVENT_STAMODE_DISCONNECTED:
            myprintf("disconnect from ssid %s, reason %d\n", evt->event_info.disconnected.ssid, evt->event_info.disconnected.reason);
            if (u8DisconnectCounter < 15)
                u8DisconnectCounter++;
            if (u8DisconnectCounter >= 15)
            {
                wifi_set_opmode(SOFTAP_MODE);

                wifi_softap_dhcps_stop();
                IP4_ADDR(&ipinfo.ip, 192, 168, 1, 1);
                IP4_ADDR(&ipinfo.gw, 192, 168, 1, 1);
                IP4_ADDR(&ipinfo.netmask, 255, 255, 255, 0);
                wifi_set_ip_info(SOFTAP_IF, &ipinfo);
                wifi_softap_dhcps_start();

                myprintf("Enter SOFTAP Mode\n");
                u8DisconnectCounter = 0;
            }
            break;

        case EVENT_STAMODE_AUTHMODE_CHANGE:
            myprintf("Authmode: %d -> %d\n", evt->event_info.auth_change.old_mode, evt->event_info.auth_change.new_mode);
            break;

        case EVENT_STAMODE_GOT_IP:
            myprintf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR, IP2STR(&evt->event_info.got_ip.ip), IP2STR(&evt->event_info.got_ip.mask), IP2STR(&evt->event_info.got_ip.gw));
            myprintf("\n");
            // Init all timers
            Time_vTimerInit();
            if(Control_u8GetAutoStart())
                HTTPC_vStartStreaming("http://hd.stream.frequence3.net/frequence3-256-z2backup.mp3", "Icy-MetaData:1\r\n", Timer_StreamingCallback);
            break;

        case EVENT_SOFTAPMODE_STACONNECTED:
            myprintf("station: " MACSTR "join, AID = %d\n", MAC2STR(evt->event_info.sta_connected.mac), evt->event_info.sta_connected.aid);
            break;

        case EVENT_SOFTAPMODE_STADISCONNECTED:
            myprintf("station: " MACSTR "leave, AID = %d\n", MAC2STR(evt->event_info.sta_disconnected.mac), evt->event_info.sta_disconnected.aid);
            break;

        default:
            break;
    }
}

void ICACHE_FLASH_ATTR set_ap(void)
{
    struct softap_config apConfig;
    struct station_config staConfig;
    struct ip_info ipinfo;
    char macaddress[17];
    unsigned char macaddr[17];
    char buff[20];
    SHA256_CTX ctx;

    // Get MAC-Address from Soft Accesspoint Interface
    wifi_get_macaddr(SOFTAP_IF, macaddr);

    /* ############################################################# */
    /* ################### Configure Station mode ################## */
    /* ############################################################# */
    os_sprintf(buff, "webradio");
    wifi_station_set_hostname(buff);
    /* ############################################################# */

    /* ############################################################# */
    /* ##################### Configure AP mode ##################### */
    /* ############################################################# */
    wifi_softap_get_config(&apConfig);
    wifi_softap_dhcps_stop();
    IP4_ADDR(&ipinfo.ip, 192, 168, 1, 1);
    IP4_ADDR(&ipinfo.gw, 192, 168, 1, 1);
    IP4_ADDR(&ipinfo.netmask, 255, 255, 255, 0);
    wifi_set_ip_info(SOFTAP_IF, &ipinfo);
    wifi_softap_dhcps_start();
    os_sprintf(buff, "WEBRADIO_%02X%02X%02X", macaddr[3], macaddr[4], macaddr[5]);
    os_memset(apConfig.ssid, 0, sizeof(apConfig.ssid));
    os_memcpy(apConfig.ssid, buff, os_strlen(buff));
    apConfig.ssid_len = os_strlen(buff);

    /* ################ Configure sha256 generator ################# */
    // Init sha256 generator and configure AP Password to be the first 4 hex values of MAC-Hash
    os_sprintf(buff, "%02x%02x%02x%02x%02x%02x", macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
    sha256_init(&ctx);
    sha256_update(&ctx, (unsigned char*) buff, os_strlen(buff));
    sha256_final(&ctx, hash);

    /* ################ Keep on configuring AP mode ################ */
    os_sprintf(buff, "%02x%02x%02x%02x", hash[0], hash[1], hash[2], hash[3]);
    os_memcpy(apConfig.password, buff, 9);
    apConfig.authmode = AUTH_WPA2_PSK;
    // For some reason channel 1 and 2 do NOT work on SOME locations?! So we use channel 3.
    apConfig.channel = 3;

    /* ################# Printout some information ################# */
    os_sprintf(macaddress, MACSTR, MAC2STR(macaddr));

    myprintf("\n");
    myprintf("\n============== AP Info =============\n");
    myprintf("OPMODE:           %u\n", wifi_get_opmode());
    myprintf("SSID:             %s\n", apConfig.ssid);
    myprintf("SSID LEN:         %d\n", apConfig.ssid_len);
    myprintf("PASSWORD:         %s\n", apConfig.password);
    myprintf("CHANNEL:          %d\n", apConfig.channel);
    myprintf("AUTHMODE:         %d\n", apConfig.authmode);
    myprintf("HIDDEN:           %d\n", apConfig.ssid_hidden);
    myprintf("MAX CONNECTION:   %d\n", apConfig.max_connection);
    myprintf("BEACON INTERVAL:  %d\n", apConfig.beacon_interval);
    myprintf("MACADDRESS:       %s\n", macaddress);
    myprintf("Web user:         admin\n");
    myprintf("Web password:     %02x%02x%02x%02x\n", hash[4], hash[5], hash[6], hash[7]);
    myprintf("\n====================================\n");
    myprintf("\n");

    wifi_softap_set_config(&apConfig);
    /* ############################################################# */

    /* ############################################################# */
    /*  Set operation mode depending if a station name has been set  */
    /* ############################################################# */
    // Get station name
    wifi_station_get_config(&staConfig);

    if (*staConfig.ssid == 0)
    {
        wifi_set_opmode(SOFTAP_MODE);
        myprintf("Enter SOFTAP Mode\n");
    }
    else
    {
        wifi_set_opmode(STATION_MODE);
        myprintf("Enter STATION Mode\n");
    }

    wifi_station_connect();

    /* ############################################################# */

    wifi_set_event_handler_cb(wifi_handle_event_cb);

    /* ############################################################# */
    /* If wifi_station_set_config is called in user_init , there is no need to call
     wifi_station_connect after that, ESP8266 will connect to router
     automatically; otherwise, need wifi_station_connect to connect.
     In general, station_config.bssid_set need to be 0, otherwise it will check
     bssid which is the MAC address of AP. */
    /* ############################################################# */
}

void ICACHE_FLASH_ATTR user_init(void)
{
    // reinit timer to enable us timer usage
    system_timer_reinit();
    // Turn debug output off
    system_set_os_print(0);
    // Init USART output
    stdoutInit();
    // set core clock to 160 MHz
    system_update_cpu_freq(SYS_CPU_160MHZ);
    // Setup wifi config for user access
    set_ap();
    // Init Esp File System with pointer to data base address
    espFsInit((void*) (webpages_espfs_start));
    // Init persistent memory
    Control_vInit();
    // Start httpd service
    httpdInit(builtInUrls, 80);
    // Init VS1053 hardware (including hardware SPI)
    VS1053_vInit();
    // Print out heap size (just for debugging purposes)
    myprintf("\nFree HEAP: %d\n", system_get_free_heap_size());

    Control_v8GetStreamList();
}
