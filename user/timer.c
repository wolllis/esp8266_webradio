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

#define USE_US_TIMER

#include <esp8266.h>
#include "vs1053.h"
#include "stdout.h"
#include "httpclient.h"

extern uint32 data_count;

os_timer_t TimerObject_1;
os_timer_t TimerObject_1000;

void ICACHE_FLASH_ATTR TimerFunc_1(void *arg)
{
    VS1053_vPoll();
}

void ICACHE_FLASH_ATTR TimerFunc_1000(void *arg)
{
    myprintf("%d Byte/s | ", data_count);
    myprintf("%d Bytes avail | ", 20000 - VS1053_u16GetFreeBufferSize());
    myprintf("Decoded Time: %d | ", VS1053_u16ReadDecodedTime());
    if (VS1053_u8ReadChannelCount() == 0)
        myprintf("Channel: Mono | ");
    if (VS1053_u8ReadChannelCount() == 1)
        myprintf("Channel: Stereo | ");
    myprintf("Samplerate: %d | ", VS1053_u16ReadSampleRate());

    switch (VS1053_u8ReadFileType())
    {
        case 0:
            myprintf("Type: None | Byterate: %d | ", VS1053_u8ReadBitRate);
            break;
        case 1:
            myprintf("Type: MP3 | ");
            break;
        case 2:
            myprintf("Type: WAV | Byterate: %d | ", VS1053_u8ReadBitRate);
            break;
        case 3:
            myprintf("Type: AAC ADTS | Byterate: %d | ", VS1053_u8ReadBitRate);
            break;
        case 4:
            myprintf("Type: AAC ADIF | Byterate: %d | ", VS1053_u8ReadBitRate);
            break;
        case 5:
            myprintf("Type: AAC .MP4 or .M4A | Byterate: %d | ", VS1053_u8ReadBitRate);
            break;
        case 6:
            myprintf("Type: WMA | Byterate: %d | ", VS1053_u8ReadBitRate);
            break;
        case 7:
            myprintf("Type: MIDI | Byterate: %d | ", VS1053_u8ReadBitRate);
            break;
        case 8:
            myprintf("Type: OGG | Byterate: %d | ", VS1053_u8ReadBitRate);
            break;
    }

    // Print out heap size (just for debugging purposes)
    myprintf("Free HEAP: %d | ", system_get_free_heap_size());
    myprintf("\n");

    data_count = 0;
}

void ICACHE_FLASH_ATTR Timer_StreamingCallback(char *response, int http_status, char *full_response)
{
    myprintf("Streaming: Stopped with Code %d\n", http_status);
    if (http_status != -1)
    {
        myprintf("Streaming: response=%s\n", full_response);
    }
}

void ICACHE_FLASH_ATTR Time_vTimerInit(void)
{
    // 500us timer
    // Faster is not possible. Otherwise callback will be called multiple times without delay
    os_timer_disarm(&TimerObject_1);
    os_timer_setfn(&TimerObject_1, (os_timer_func_t*) TimerFunc_1, NULL);
    os_timer_arm_us(&TimerObject_1, 500, 1);
    // 1000ms timer
    os_timer_disarm(&TimerObject_1000);
    os_timer_setfn(&TimerObject_1000, (os_timer_func_t*) TimerFunc_1000, NULL);
    os_timer_arm(&TimerObject_1000, 1000, 1);
}
