#ifndef CONTROL_H_
#define CONTROL_H_

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

typedef struct
{
        int8 TrebleAmp; // 1.5 dB steps (-8..7, 0 = off)
        uint8 TrebleLim; // frequency in 1000 Hz steps (1..15)
        int8 BassAmp; // 1 dB steps (0..15, 0 = off)
        int8 BassLim; // 10 Hz steps (2..15)
} Control_tstEnhancerSettings;

typedef enum
{
    Control_enSpartialProcessing_Off,
    Control_enSpartialProcessing_Minimal,
    Control_enSpartialProcessing_Normal,
    Control_enSpartialProcessing_Extreme
} Control_tenSpartialProcessing;

char stream_name[3][30];
char stream_address[3][100];

void ICACHE_FLASH_ATTR Control_vInit(void);
void ICACHE_FLASH_ATTR Control_vSetVolume(uint8 value);
uint8 ICACHE_FLASH_ATTR Control_u8GetVolume(void);
void ICACHE_FLASH_ATTR Control_vSetEnhancer(Control_tstEnhancerSettings *data);
void ICACHE_FLASH_ATTR Control_vGetEnhancer(Control_tstEnhancerSettings *data);
void ICACHE_FLASH_ATTR Control_vSetAutoStart(uint8 value);
uint8 ICACHE_FLASH_ATTR Control_u8GetAutoStart(void);
void ICACHE_FLASH_ATTR Control_vSetSpartialProcessingLevel(Control_tenSpartialProcessing Level);
Control_tenSpartialProcessing ICACHE_FLASH_ATTR Control_u8GetSpartialProcessingLevel(void);
void ICACHE_FLASH_ATTR Control_v8GetStreamList(void);

#endif /* CONTROL_H_ */
