#ifndef USER_VS1053_H_
#define USER_VS1053_H_

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

/* Connection
 D3 (GPIO_0)  => XRESET	Active low asynchronous reset
 D2 (GPIO_4)  => DREQ		Data request signal
 D6 => SO		Serial output. In reads, data is shifted out on the falling SCK edge.
 D7 => SI		Serial input. If a chip select is active, SI is sampled on the rising CLK edge.
 D5 => SCLK		Serial clock input
 D1 (GPIO_5)  => XDCS		Data chip select (active low)
 D8 (GPIO_15) => XCS		Chip select input (active low)
 */

void ICACHE_FLASH_ATTR VS1053_vInit(void);
void ICACHE_FLASH_ATTR VS1053_vTest(void);
uint32 ICACHE_FLASH_ATTR VS1053_u32SendMusicData(uint8 *data);
uint8_t ICACHE_FLASH_ATTR VS1053_vFillRingBuffer(uint8 *data, uint32 length);
uint8 ICACHE_FLASH_ATTR VS1053_s16ReadRingBuffer(uint8 *data);
uint16 ICACHE_FLASH_ATTR VS1053_u16GetFreeBufferSize(void);
void ICACHE_FLASH_ATTR VS1053_vPoll(void);
void ICACHE_FLASH_ATTR VS1053_vSetVolume(uint8 vol_left, uint8 vol_right);
uint16 ICACHE_FLASH_ATTR VS1053_u16ReadDecodedTime(void);
uint8 ICACHE_FLASH_ATTR VS1053_u8ReadChannelCount(void);
uint16 ICACHE_FLASH_ATTR VS1053_u16ReadSampleRate(void);
uint8 ICACHE_FLASH_ATTR VS1053_u8ReadFileType(void);
uint16 ICACHE_FLASH_ATTR VS1053_u8ReadBitRate(void);
void ICACHE_FLASH_ATTR VS1053_vSetEnhancer(int8 Trebel_Amplitude, uint8 Trebel_Limit, uint8 Bass_Amplitude, uint8 Bass_Limit);
void ICACHE_FLASH_ATTR VS1053_vSetSpartialProcessing(uint8 Level);

#endif /* USER_VS1053_H_ */
