/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Martin d'Allens <martin.dallens@gmail.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "limits.h"
#include "httpclient.h"
#include "vs1053.h"

// Debug output.
#if 1
#define PRINTF(...) myprintf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

typedef struct
{
    char *path;
    int port;
    char *post_data;
    char *headers;
    char *hostname;
    char *buffer;
    int buffer_size;
    bool secure;
    http_callback user_callback;
    bool buffer_wait_timer_elapsed;
    bool header_received;
    uint32 music_bytes_until_next_icycast;
    uint32 icycast_info_interval;
} request_args;

uint32 data_count = 0;
bool StopStreaming = 0;
bool SteamStarted = 0;
os_timer_t HTTPC_TimerObject;

static char* ICACHE_FLASH_ATTR esp_strdup(const char *str)
{
    //PRINTF("-----esp_strdup----\r\n");
    if (str == NULL)
    {
        return NULL;
    }
    char *new_str = (char*) os_malloc(os_strlen(str) + 1); // 1 for null character
    if (new_str == NULL)
    {
        //PRINTF("esp_strdup: malloc error");
        return NULL;
    }
    os_strcpy(new_str, str);
    return new_str;
}

static int ICACHE_FLASH_ATTR HTTPC_iChunkedDecode(const char *chunked, char *decode)
{
    //PRINTF("-----chunked_decode----\r\n");

    int i = 0, j = 0;
    int decode_size = 0;
    char *str = (char*) chunked;
    do
    {
        char *endstr = NULL;
        //[chunk-size]
        i = Stdout_s32Strtol(str + j, endstr, 16);
        //PRINTF("Chunk Size:%d\r\n", i);
        if (i <= 0)
            break;
        //[chunk-size-end-ptr]
        endstr = (char*) os_strstr(str + j, "\r\n");
        //[chunk-ext]
        j += endstr - (str + j);
        //[CRLF]
        j += 2;
        //[chunk-data]
        decode_size += i;
        os_memcpy((char*) &decode[decode_size - i], (char*) str + j, i);
        j += i;
        //[CRLF]
        j += 2;
    } while (true);

    //
    //footer CRLF
    //

    return j;
}

static void ICACHE_FLASH_ATTR HTTPC_vSearchMetadata(char *buf, request_args *req, unsigned short header_len)
{
    char *ptr;

    // Search for field "icy-metaint:" in data packet
    ptr = strstr(buf, "icy-metaint:");
    // Check if it has been found
    if (ptr != NULL)
    {
        // Point to field value rather than start of "icy-metaint:"
        ptr += strlen("icy-metaint:");
        // Save information for later use
        req->icycast_info_interval = strtol(ptr, NULL, 0);
        // Field "icy-metaint:" has been found
        PRINTF("HTTPC: icy-metaint: is %d\n", req->icycast_info_interval);
    }
    else
    {
        // No field "icy-metaint:" found in data packet
        // Leave icycast_info_interval as it is (should be initialized with zero)
        PRINTF("HTTPC: icy-metaint: has not been found\n");
    }
}

static void ICACHE_FLASH_ATTR HTTPC_vTimerCallback(void *arg)
{
    // Timer has elapsed. Now we are allowed to buffer music data
    request_args *req = (request_args*) arg;
    req->buffer_wait_timer_elapsed = 1;
}

static void ICACHE_FLASH_ATTR HTTPC_vFillBuffer(request_args *req, char *buf, unsigned short len)
{
    uint8 result;

    if (req->buffer_wait_timer_elapsed == 0)
    {
        // Buffer fill timer not jet elapsed
        // This is to wait for stable data rate from streaming server before starting to buffer music data
        return;
    }

    // Fill audio buffer with music data
    result = VS1053_vFillRingBuffer(buf, len);
    // Check for success
    if (result == 0)
    {
        // Ringbuffer has no more free space.
        // At this point it is unavoidably to lose music data because there is no place to store it...
        PRINTF("HTTPC: VS1053 Ringbuffer full!\n");
    }
    // For transfer rate calculation
    data_count += len;
}

static void ICACHE_FLASH_ATTR HTTPC_vReceiveCallback(void *arg, char *buf, unsigned short len)
{
    struct espconn *conn = (struct espconn*) arg;
    request_args *req = (request_args*) conn->reverse;
    static uint8 first_message = 1;
    char *ptr;

    //PRINTF("HTTPC: Data has been received | Size: %d Bytes\n", len);

    if (StopStreaming == 1)
    {
        // Stream should be stopped. So we directly disconnect and return here.
        PRINTF("HTTPC: Stream should be stopped\n");
        if (req->secure)
            espconn_secure_disconnect(conn);
        else
            espconn_disconnect(conn);
        // The disconnect callback will be called
        return;
    }

    if (req->header_received == 0)
    {
        // First data received by client
        // Maybe not the best idea to use specific timer in multi instance application :/
        os_timer_disarm(&HTTPC_TimerObject);
        os_timer_setfn(&HTTPC_TimerObject, (os_timer_func_t*) HTTPC_vTimerCallback, req);
        os_timer_arm(&HTTPC_TimerObject, 2000, 0);

        // Pointer to music data
        ptr = strstr(buf, "\r\n\r\n");
        // Check if end of HEADER has been found
        if (ptr != NULL)
        {
            // Point to end of header rather than start of "\r\n\r\n"
            ptr += 4;
            // Calculate music data length
            len -= ptr - buf;
            // Search for ICYCAST metadata in data packet
            HTTPC_vSearchMetadata(buf, req, ptr - buf);
            req->music_bytes_until_next_icycast = req->icycast_info_interval;
        }
        else
        {
            // No HEADER detected? Something went wrong
            PRINTF("HTTPC: First received data does not contain a HEADER!\n");
            if (req->secure)
                espconn_secure_disconnect(conn);
            else
                espconn_disconnect(conn);
            // The disconnect callback will be called
            return;
        }
        // Enter only once
        req->header_received = 1;
    }
    else
    {
        // Not the first data received by the client
        ptr = buf;
    }

    // Check for ICYCAST Metadata available
    if (req->icycast_info_interval != 0)
    {
        // The server injects ICYCAST metadata in music stream
        if (len > req->music_bytes_until_next_icycast)
        {
            // This packet contains metadata
            uint32 mp3_bytes_after_metadata;
            uint32 mp3_bytes_before_metadata;
            uint32 metadata_bytes;
            char *metadata_ptr;
            char *mp3_after_metadata_ptr;

            // Get location of Metadata
            metadata_ptr = ptr + (req->music_bytes_until_next_icycast);
            mp3_bytes_before_metadata = req->music_bytes_until_next_icycast;
            metadata_bytes = (*metadata_ptr) * 16 + 1;
            mp3_bytes_after_metadata = len - (req->music_bytes_until_next_icycast) - metadata_bytes;
            mp3_after_metadata_ptr = metadata_ptr + metadata_bytes;

            // Check for empty Metadata
            if (*metadata_ptr != 0)
            {
                char *buffer = (char*) os_malloc(metadata_bytes);
                // Check if memory could be allocated
                if (buffer != NULL)
                {
                    // First byte contains length and can be skipped for information
                    os_memcpy(buffer, metadata_ptr + 1, metadata_bytes - 1);
                    //Terminate string
                    buffer[metadata_bytes - 1] = '\0';
                    // Do something with metadata_pointer and metadata_length
                    myprintf("HTTPC: %s\n", buffer);
                    // Free unused buffer
                    os_free(buffer);
                }
            }

            // Send data before metadata to buffer
            HTTPC_vFillBuffer(req, ptr, mp3_bytes_before_metadata);
            // Send data after metadata to buffer
            HTTPC_vFillBuffer(req, mp3_after_metadata_ptr, mp3_bytes_after_metadata);
            // Calculate position of next metadata
            req->music_bytes_until_next_icycast = (req->icycast_info_interval) - mp3_bytes_after_metadata;
        }
        else
        {
            req->music_bytes_until_next_icycast -= len;
            // All the data is mp3 => Send to buffer
            HTTPC_vFillBuffer(req, ptr, len);
        }
    }
    else
    {
        // All the data is mp3 => Send to buffer
        HTTPC_vFillBuffer(req, ptr, len);
    }
}

static void ICACHE_FLASH_ATTR HTTPC_vSentCallback(void *arg)
{
    struct espconn *conn = (struct espconn*) arg;
    request_args *req = (request_args*) conn->reverse;

    PRINTF("HTTPC: Data has been sent\n");

    // Check for remaining data
    if (req->post_data == NULL)
    {
        PRINTF("HTTPC: All data has been sent\n");
    }
    else
    {
        // The headers were sent, now send the contents.
        PRINTF("HTTPC: Sending BODY\n");

        // Handle connection differently depending if SSL is used or not
        if (req->secure)
            espconn_secure_sent(conn, (uint8_t*) req->post_data, strlen(req->post_data));
        else
            espconn_sent(conn, (uint8_t*) req->post_data, strlen(req->post_data));
    }
}

static void ICACHE_FLASH_ATTR HTTPC_vConnectCallback(void *arg)
{

    struct espconn *conn = (struct espconn*) arg;
    request_args *req = (request_args*) conn->reverse;
    const char *method = "GET";
    char post_headers[32] = "";

    PRINTF("HTTPC: Connected\n");

    // Configure receive and sent callback
    espconn_regist_recvcb(conn, HTTPC_vReceiveCallback);
    espconn_regist_sentcb(conn, HTTPC_vSentCallback);

    // If there is data this is a POST request.
    if (req->post_data != NULL)
    {
        method = "POST";
        os_sprintf(post_headers, "Content-Length: %d\r\n", strlen(req->post_data));
    }

    char buf[69 + strlen(method) + strlen(req->path) + strlen(req->hostname) + strlen(req->headers) + strlen(post_headers)];
    int len = os_sprintf(buf, "%s %s HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "Connection: close\r\n"
            "User-Agent: ESP8266\r\n"
            "%s"
            "%s"
            "\r\n", method, req->path, req->hostname, req->port, req->headers, post_headers);

    PRINTF("HTTPC: Sending HEADER\n");

    // Handle connection differently depending if SSL is used or not
    if (req->secure)
        espconn_secure_sent(conn, (uint8_t*) buf, len);
    else
        espconn_sent(conn, (uint8_t*) buf, len);

}

static void ICACHE_FLASH_ATTR HTTPC_vDisconnectCallback(void *arg)
{
    struct espconn *conn = (struct espconn*) arg;
    request_args *req = (request_args*) conn->reverse;
    int http_status = -1;
    char *body = "";

    PRINTF("HTTPC: Disconnected\n");
    if (conn == NULL)
    {
        // No further information available. Also no data to free here. Strange...
        return;
    }

    if (req != NULL)
    {
        const char *version = "HTTP/1.1 ";

        // Check for hostname buffer
        if (req->buffer == NULL)
        {
            // No data available
            PRINTF("HTTPC: Hostname buffer shouldn't be NULL\n");
        }
        else
            if (req->buffer[0] != '\0')
            {
                // Check for HTTP Version
                if (os_strncmp(req->buffer, version, strlen(version)) != 0)
                {
                    PRINTF("HTTPC: Invalid HTTP version= %s\n", req->buffer);
                }
                else
                {
                    http_status = strtol(req->buffer + strlen(version), NULL, 0);
                    body = (char*) os_strstr(req->buffer, "\r\n\r\n") + 4;
                    if (os_strstr(req->buffer, "Transfer-Encoding: chunked"))
                    {
                        int body_size = req->buffer_size - (body - req->buffer);
                        char chunked_decode_buffer[body_size];
                        os_memset(chunked_decode_buffer, 0, body_size);
                        // Chunked data
                        HTTPC_iChunkedDecode(body, chunked_decode_buffer);
                        os_memcpy(body, chunked_decode_buffer, body_size);
                    }
                }
            }

        // Reset abort variable
        StopStreaming = 0;
        SteamStarted = 0;

        // Callback is optional
        if (req->user_callback != NULL)
        {
            // Call user callback
            req->user_callback(body, http_status, req->buffer);
        }

        // Data needs to be freed because it has been allocated before
        os_free(req->buffer);
        os_free(req->hostname);
        os_free(req->path);

        // Data needs to be freed because it has been allocated before
        if (req->headers != NULL)
        {
            os_free(req->headers);
            req->headers = NULL;
        }

        // Data needs to be freed because it has been allocated before
        if (req->post_data != NULL)
        {
            os_free(req->post_data);
            req->post_data = NULL;
        }

        os_free(req);
    }

    // Disconnect from host
    espconn_delete(conn);

    if (conn->proto.tcp != NULL)
    {
        // Data needs to be freed because it has been allocated before
        os_free(conn->proto.tcp);
        conn->proto.tcp = NULL;
    }

    // Data needs to be freed because it has been allocated before
    os_free(conn);
}

static void ICACHE_FLASH_ATTR HTTPC_vErrorCallback(void *arg, sint8 errType)
{
    PRINTF("HTTPC: Disconnected because of error\n");
    // Handle connection abort the same way as a "normal" disconnect
    HTTPC_vDisconnectCallback(arg);
}

static void ICACHE_FLASH_ATTR HTTPC_vDnsCallback(const char *hostname, ip_addr_t *addr, void *arg)
{
    request_args *req = (request_args*) arg;

    // Check for valid address data
    if (addr == NULL)
    {
        // Invalid hostname or something went wrong
        PRINTF("HTTPC: DNS failed for= %s\n", hostname);

        // Call user callback with invalid arguments
        if (req->user_callback != NULL)
        {
            req->user_callback("", -1, "");
        }

        // Data needs to be freed because it has been allocated before
        os_free(req);
    }
    else
    {
        // Hostname has been resolved
        PRINTF("HTTPC: Hostname (%s) resolved to " IPSTR "\n", hostname, IP2STR(addr));

        // Allocate memory for client connection
        struct espconn *conn = (struct espconn*) os_malloc(sizeof(struct espconn));

        // Prepare client connection (use req for hostname information)
        conn->type = ESPCONN_TCP;
        conn->state = ESPCONN_NONE;
        conn->proto.tcp = (esp_tcp*) os_malloc(sizeof(esp_tcp));
        conn->proto.tcp->local_port = espconn_port();
        conn->proto.tcp->remote_port = req->port;
        conn->reverse = req;

        os_memcpy(conn->proto.tcp->remote_ip, addr, 4);

        // Set connection callbacks
        espconn_regist_connectcb(conn, HTTPC_vConnectCallback);
        espconn_regist_disconcb(conn, HTTPC_vDisconnectCallback);
        espconn_regist_reconcb(conn, HTTPC_vErrorCallback);

        // Handle connection differently depending if SSL is used or not
        if (req->secure)
        {
            // set SSL buffer size
            espconn_secure_set_size(ESPCONN_CLIENT, 5120);
            espconn_secure_connect(conn);
        }
        else
        {
            espconn_connect(conn);
        }
    }
}

static void ICACHE_FLASH_ATTR HTTPC_vPrepareRequest(const char *hostname, int port, bool secure, const char *path, const char *post_data, const char *headers, http_callback user_callback)
{
    request_args *req = (request_args*) os_malloc(sizeof(request_args));
    req->hostname = esp_strdup(hostname);
    req->path = esp_strdup(path);
    req->port = port;
    req->secure = secure;
    req->headers = esp_strdup(headers);
    req->post_data = esp_strdup(post_data);
    req->buffer_size = 1;
    req->buffer = (char*) os_malloc(1);
    req->buffer[0] = '\0'; // Empty string.
    req->user_callback = user_callback;
    req->header_received = 0;
    req->music_bytes_until_next_icycast = 0;
    req->icycast_info_interval = 0;
    req->buffer_wait_timer_elapsed = 0;

    ip_addr_t addr;

    PRINTF("HTTPC: Resolve hostname using DNS\n");

    // It seems we don't need a real espconn pointer here.
    err_t error = espconn_gethostbyname((struct espconn*) req, hostname, &addr, HTTPC_vDnsCallback);

    switch (error)
    {
        case ESPCONN_INPROGRESS:
            PRINTF("HTTPC: Waiting for DNS to resolve hostname\n");
            break;
        case ESPCONN_OK:
            // Already in the local names table (or hostname was an IP address), execute the callback ourselves.
            PRINTF("HTTPC: Hostname already resolved => Don't need additional DNS request\n");
            HTTPC_vDnsCallback(hostname, &addr, req);
            break;
        case ESPCONN_ARG:
            PRINTF("HTTPC: Hostname error= %s\n", hostname);
            // Handle all DNS errors the same way.
            HTTPC_vDnsCallback(hostname, NULL, req);
            break;
        default:
            PRINTF("HTTPC: DNS error code= %d\n", error);
            // Handle all DNS errors the same way.
            HTTPC_vDnsCallback(hostname, NULL, req);
            break;
    }
}

static void ICACHE_FLASH_ATTR HTTPC_vSendPost(const char *url, const char *post_data, const char *headers, http_callback user_callback)
{
    char hostname[128] = "";
    int port;
    bool secure;

    if (os_strncmp(url, "http://", strlen("http://")) == 0)
    {
        port = 80;
        secure = false;
        // Get rid of the protocol.
        url += strlen("http://");
    }
    else
        if (os_strncmp(url, "https://", strlen("https://")) == 0)
        {
            port = 443;
            secure = true;
            // Get rid of the protocol.
            url += strlen("https://");
        }
        else
        {
            PRINTF("HTTPC: URL is not HTTP or HTTPS= %s\n", url);
            return;
        }

    // find first occurrence of '/' and returns a pointer on it
    char *path = os_strchr(url, '/');
    // No path character found?
    if (path == NULL)
    {
        // Set pointer to end of string
        path = os_strchr(url, '\0');
    }

    // find first occurrence of ':' and returns a pointer on it
    char *colon = os_strchr(url, ':');
    // No colon character found?
    if (colon == NULL)
    {
        // The port is not present. URL is contains hostname + path
        os_memcpy(hostname, url, path - url);
        // Terminate string
        hostname[path - url] = '\0';
    }
    // Colon character found
    else
    {
        // colon points on port number
        port = strtol(colon + 1, NULL, 0);
        // Check if port was correctly specified
        if (port == 0)
        {
            PRINTF("HTTPC: Port error= %s\n", url);
            return;
        }

        // The port is present. URL contains hostname + colon
        os_memcpy(hostname, url, colon - url);
        // Terminate string
        hostname[colon - url] = '\0';
    }

    // Empty 'path' is not allowed
    if (path[0] == '\0')
        path = "/";

    // Some debug output
    PRINTF("HTTPC: hostname=%s | port=%d | path=%s\n", hostname, port, path);

    // Create request based on url data
    HTTPC_vPrepareRequest(hostname, port, secure, path, post_data, headers, user_callback);
}

void ICACHE_FLASH_ATTR HTTPC_vStartStreaming(const char *url, const char *headers, http_callback user_callback)
{
    if (SteamStarted == 1)
        return;
    SteamStarted = 1;
    // Open stream by sending POST
    HTTPC_vSendPost(url, NULL, headers, user_callback);
}

void ICACHE_FLASH_ATTR HTTPC_vStopStreaming(void)
{
    // End Stream after next received data packet
    StopStreaming = 1;
}
