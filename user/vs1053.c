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
#include "vs1053.h"
#include "spi.h"
#include "control.h"

/******* GENERAL COMMANDS *******/
#define VS_WRITE_COMMAND 	0x0200
#define VS_READ_COMMAND 	0x0300

/********* MODE REGISTER ********/
// Register Address
#define SCI_MODE        	0x0000
// Register Content
#define SM_DIFF         	0x0001
#define SM_LAYER12			0x0002
#define SM_RESET        	0x0004
#define SM_CANCEL           0x0008
#define SM_EARSPEAKER_LO   	0x0010
#define SM_TESTS        	0x0020
#define SM_STREAM       	0x0040
#define SM_EARSPEAKER_HI   	0x0080
#define SM_DACT         	0x0100
#define SM_SDIORD       	0x0200
#define SM_SDISHARE     	0x0400
#define SM_SDINEW       	0x0800
#define SM_ADPCM        	0x1000
//#define SM_ADPCM_HP     	0x2000
#define SM_LINE1            0x4000
#define SM_CLK_RANGE        0x8000
/********************************/

/******** STATUS REGISTER *******/
// Register Address
#define SCI_STATUS        	0x0001
// Register Content
#define SS_REFERENCE_SEL   	0x0001
#define SS_AD_CLOCK			0x0002
#define SS_APDOWN1        	0x0004
#define SS_APDOWN2          0x0008
#define SS_VER_1			0x0010
#define SS_VER_2        	0x0020
#define SS_VER_3       		0x0040
#define SS_VER_4			0x0080
//#define RESERVED         	0x0100
//#define RESERVED       	0x0200
#define SS_VCM_DISABLE     	0x0400
#define SS_VCM_OVERLOAD    	0x0800
#define SS_SWING_1        	0x1000
#define SS_SWING_2     		0x2000
#define SS_SWING_3          0x4000
#define SS_DO_NOT_JUMP      0x8000
/********************************/

/******** BASS REGISTER *******/
// Register Address
#define SCI_BASS        	0x0002
// Register Content
#define SB_FREQLIMIT_1   	0x0001
#define SB_FREQLIMIT_2  	0x0002
#define SB_FREQLIMIT_3    	0x0004
#define SB_FREQLIMIT_4      0x0008
#define SB_AMPLITUDE_1		0x0010
#define SB_AMPLITUDE_2  	0x0020
#define SB_AMPLITUDE_3 		0x0040
#define SB_AMPLITUDE_4		0x0080
#define ST_FREQLIMIT_1     	0x0100
#define ST_FREQLIMIT_2     	0x0200
#define ST_FREQLIMIT_3     	0x0400
#define ST_FREQLIMIT_4    	0x0800
#define ST_AMPLITUDE_1     	0x1000
#define ST_AMPLITUDE_2		0x2000
#define ST_AMPLITUDE_3      0x4000
#define ST_AMPLITUDE_4      0x8000
/********************************/

/********* CLOCK REGISTER *******/
// Register Address
#define SCI_CLOCKF        	0x0003
// Register Content
#define SC_FREQ_1   	    0x0001
#define SC_FREQ_2  			0x0002
#define SC_FREQ_3    		0x0004
#define SC_FREQ_4      		0x0008
#define SC_FREQ_5			0x0010
#define SC_FREQ_6  			0x0020
#define SC_FREQ_7 			0x0040
#define SC_FREQ_8			0x0080
#define SC_FREQ_9     		0x0100
#define SC_FREQ_10     		0x0200
#define SC_FREQ_11     		0x0400
#define SC_ADD_1    		0x0800
#define SC_ADD_2     		0x1000
#define SC_MULT_1			0x2000
#define SC_MULT_2      		0x4000
#define SC_MULT_3      		0x8000
/********************************/

/***** DECODE TIME REGISTER *****/
// Register Address
#define SCI_DECODE_TIME    	0x0004
/********************************/

/***** AUDATA REGISTER *****/
// Register Address
#define SCI_AUDATA    		0x0005
// Register Content
#define AD_STEREO   	    0x0001
#define AD_SRATE_1			0x0002
#define AD_SRATE_2    		0x0004
#define AD_SRATE_3      	0x0008
#define AD_SRATE_4			0x0010
#define AD_SRATE_5  		0x0020
#define AD_SRATE_6 			0x0040
#define AD_SRATE_7			0x0080
#define AD_SRATE_8     		0x0100
#define AD_SRATE_9     		0x0200
#define AD_SRATE_10     	0x0400
#define AD_SRATE_11    		0x0800
#define AD_SRATE_12    		0x1000
#define AD_SRATE_13			0x2000
#define AD_SRATE_14    		0x4000
#define AD_SRATE_15    		0x8000
/********************************/

/******** HDAT0 REGISTER ********/
// Register Address
#define SCI_HDAT0			0x0008
/********************************/

/******** HDAT1 REGISTER ********/
// Register Address
#define SCI_HDAT1			0x0009
/********************************/

/******* VOLUME REGISTER ********/
// Register Address
#define SCI_VOL	    		0x000B
// Register Content
#define VOL_RIGHT_1   		0x0001
#define VOL_RIGHT_2			0x0002
#define VOL_RIGHT_3    		0x0004
#define VOL_RIGHT_4     	0x0008
#define VOL_RIGHT_5			0x0010
#define VOL_RIGHT_6  		0x0020
#define VOL_RIGHT_7 		0x0040
#define VOL_RIGHT_8			0x0080
#define VOL_LEFT_1     		0x0100
#define VOL_LEFT_2     		0x0200
#define VOL_LEFT_3     		0x0400
#define VOL_LEFT_4    		0x0800
#define VOL_LEFT_5    		0x1000
#define VOL_LEFT_6			0x2000
#define VOL_LEFT_7    		0x4000
#define VOL_LEFT_8    		0x8000
/********************************/

#define BUFFER_FAIL     0
#define BUFFER_SUCCESS  1
#define BUFFER_SIZE 20000

#define MUTE        1
#define UNMUTE      2
#define UNMUTE_TIME 200000

struct Buffer
{
    uint8_t data[BUFFER_SIZE];
    uint16_t read; // zeigt auf das Feld mit dem aeltesten Inhalt
    uint16_t write; // zeigt immer auf leeres Feld
} buffer = { { }, 0, 0 };

static void ICACHE_FLASH_ATTR VS1053_vHardwareReset(void)
{
    // Set Pin D3 (GPIO_0) low
    GPIO_OUTPUT_SET(0, 0);
    // Wait for reset to be processed
    while (GPIO_INPUT_GET(4));
    // Set Pin D3 (GPIO_0) high
    GPIO_OUTPUT_SET(0, 1);
    // Check for init complete on DREQ pin (rises when done)
    while (!GPIO_INPUT_GET(4));
}

static void ICACHE_FLASH_ATTR VS1053_vChipSelectData(bool value)
{
    // Invert value
    GPIO_OUTPUT_SET(5, !value);
}

static void ICACHE_FLASH_ATTR VS1053_vChipSelectCommand(bool value)
{
    // Invert value
    GPIO_OUTPUT_SET(15, !value);
}

static void ICACHE_FLASH_ATTR VS1053_vWriteRegister(uint16 address, uint16 value)
{
    // Check if slave is ready to receive data
    while (!GPIO_INPUT_GET(4));
    // Chip select for command instruction
    VS1053_vChipSelectCommand(1);
    spiwrite_32(((VS_WRITE_COMMAND | address) << 16) | value);
    VS1053_vChipSelectCommand(0);
    // Wait for command to be processed
    while (!GPIO_INPUT_GET(4));
}

static uint16 ICACHE_FLASH_ATTR VS1053_u16ReadRegister(uint16 address)
{
    uint32 result;
    // Check if slave is ready to receive data
    while (!GPIO_INPUT_GET(4));
    // Chip select for command instruction
    VS1053_vChipSelectCommand(1);
    spiwrite_16(VS_READ_COMMAND | address);
    result = spiwrite_16(0x0000);
    VS1053_vChipSelectCommand(0);
    // Wait for command to be processed
    while (!GPIO_INPUT_GET(4));
    return result;
}

static void ICACHE_FLASH_ATTR VS1053_vWriteData(uint32 value)
{
    // Check if slave is ready to receive data
    while (!GPIO_INPUT_GET(4));
    // Chip select for data
    VS1053_vChipSelectData(1);
    spiwrite_32(value);
    VS1053_vChipSelectData(0);
    // Wait for command to be processed
    while (!GPIO_INPUT_GET(4));
}

void ICACHE_FLASH_ATTR VS1053_vInit(void)
{
    uint8 temp;
    Control_tstEnhancerSettings data_pointer;
    // Init onboard LED
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
    // Use Pin D2 (GPIO_4) for DREQ input
    GPIO_DIS_OUTPUT(4);
    // Init gpio output
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);
    // Set CS Signals high
    GPIO_OUTPUT_SET(5, 1);
    GPIO_OUTPUT_SET(15, 1);
    // Init SPI interface with 1MHz clock
    spi_init(HSPI);
    // Reset VS1053
    VS1053_vHardwareReset();
    // Update clock register
    // SC_MULT = x5.0, SC_ADD= x2.0 => Clock is then 12,288 MHz x 5.0 = 61,44 MHz
    VS1053_vWriteRegister(SCI_CLOCKF, 0xF800);
    // Change SPI clock to 13,33 MHz
    // This basically the fastest frequency for transmitting data to VS1053
    spi_clock(HSPI, 3, 2);
    // Update mode
    VS1053_vWriteRegister(SCI_MODE, SM_SDINEW | SM_LINE1 | SM_LAYER12);

    // Calculate volume
    temp = 0xFF-((Control_u8GetVolume()*5)/2);
    // Set audio parameters based on stored values
    VS1053_vSetVolume(temp,temp);
    Control_vGetEnhancer(&data_pointer);
    VS1053_vSetEnhancer(
        data_pointer.TrebleAmp,
        data_pointer.TrebleLim,
        data_pointer.BassAmp,
        data_pointer.BassLim);
    VS1053_vSetSpartialProcessing(Control_u8GetSpartialProcessingLevel());
}

void ICACHE_FLASH_ATTR VS1053_vTest(void)
{
    // Hardware reset needed before executing any tests
    VS1053_vHardwareReset();
    // Set volume to apropriate level
    VS1053_vWriteRegister(SCI_VOL, 0x7F7F);
    // Enter simple sine test
    VS1053_vWriteRegister(SCI_MODE, SM_TESTS | SM_SDINEW);
    // In order to enter test mode the following "magic" data has to be written into data buffer
    VS1053_vWriteData(0x53EF6e03);
    VS1053_vWriteData(0x00000000);
}

uint32 ICACHE_FLASH_ATTR VS1053_u32SendMusicData(uint8 *data)
{
    if (!GPIO_INPUT_GET(4))
    {
        // Chip is not ready jet
        return 0;
    }
    else
    {
        // Chip select for data
        VS1053_vChipSelectData(1);
        // We are allowed to send at least 32Bytes of Data
        for (int i = 0; i < 8; i++)
        {
            spiwrite_32((data[i * 4] << 24) | (data[(i * 4) + 1] << 16) | (data[(i * 4) + 2] << 8) | (data[(i * 4) + 3]));
        }
        // Chip select for data
        VS1053_vChipSelectData(0);
        return 1;
    }
}

static ICACHE_FLASH_ATTR uint8_t BufferIn(uint8_t byte)
{
    if ((buffer.write + 1 == buffer.read) || (buffer.read == 0 && buffer.write + 1 == BUFFER_SIZE))
        return BUFFER_FAIL; // voll

    buffer.data[buffer.write] = byte;

    buffer.write++;
    if (buffer.write >= BUFFER_SIZE)
        buffer.write = 0;

    return BUFFER_SUCCESS;
}

static ICACHE_FLASH_ATTR uint8_t BufferOut(uint8_t *pByte)
{
    if (buffer.read == buffer.write)
        return BUFFER_FAIL;

    *pByte = buffer.data[buffer.read];

    buffer.read++;
    if (buffer.read >= BUFFER_SIZE)
        buffer.read = 0;

    return BUFFER_SUCCESS;
}

uint8_t ICACHE_FLASH_ATTR VS1053_vFillRingBuffer(uint8 *data, uint32 length)
{
    uint8 temp;
    for (uint32 i = 0; i < length; i++)
    {
        if (BufferIn(*(data + i)) == BUFFER_FAIL)
        {
            return BUFFER_FAIL;
        }
    }
    return BUFFER_SUCCESS;
}

uint8 ICACHE_FLASH_ATTR VS1053_s16ReadRingBuffer(uint8 *data)
{
    return BufferOut(data);
}

uint16 ICACHE_FLASH_ATTR VS1053_u16GetFreeBufferSize(void)
{
    if (buffer.write == buffer.read)
        return BUFFER_SIZE;
    if (buffer.write < buffer.read)
        return buffer.read - buffer.write;
    if (buffer.write > buffer.read)
        return (BUFFER_SIZE - buffer.write) + buffer.read;
    return 0;
}

uint16 ICACHE_FLASH_ATTR VS1053_u16GetUsedBufferSize(void)
{
    return (BUFFER_SIZE - VS1053_u16GetFreeBufferSize());
}

void ICACHE_FLASH_ATTR VS1053_vPoll(void)
{
    static uint8 data[32];
    static uint8 data_pending = 0;
    uint8 temp;

    // Check if enough data is available in buffer
    if (VS1053_u16GetUsedBufferSize() >= 32)
    {
        // Enough data available
        if (!data_pending)
        {
            // There is no data pending
            // Read 32 Byte from ringbuffer
            for (uint32 i = 0; i < 32; i++)
                VS1053_s16ReadRingBuffer(&data[i]);
        }
        // Try to send data to VS1053 (function call will return zero in case VS1053 is not ready)
        temp = VS1053_u32SendMusicData(data);
        // Check if data has been send
        if (temp == 0)
        {
            // Data was not send to VS1053. Try again next time.
            data_pending = 1;
        }
        else
        {
            // Data has been send to VS1053
            data_pending = 0;
        }
    }
}

void ICACHE_FLASH_ATTR VS1053_vSetVolume(uint8 vol_left, uint8 vol_right)
{
    // Directly write to volume register
    VS1053_vWriteRegister(SCI_VOL, (vol_left << 8) | (vol_right));
}

uint16 ICACHE_FLASH_ATTR VS1053_u16ReadDecodedTime(void)
{
    return VS1053_u16ReadRegister(SCI_DECODE_TIME);
}

uint8 ICACHE_FLASH_ATTR VS1053_u8ReadChannelCount(void)
{
    return (AD_STEREO & VS1053_u16ReadRegister(SCI_AUDATA));
}

uint16 ICACHE_FLASH_ATTR VS1053_u16ReadSampleRate(void)
{
    return (VS1053_u16ReadRegister(SCI_AUDATA) & 0xFFFE);
}

uint8 ICACHE_FLASH_ATTR VS1053_u8ReadFileType(void)
{
    uint16 temp = VS1053_u16ReadRegister(SCI_HDAT1);

    if ((temp >= 0xFFE0) && (temp <= 0xFFFF)) // mp3
        return 1;

    switch (temp)
    {
        case 0x7665: // WAV
            return 2;
        case 0x4154: // AAC ADTS
            return 3;
        case 0x4144: // AAC ADIF
            return 4;
        case 0x4D34: // AAC .mp4 / .m4a
            return 5;
        case 0x574D: // WMA
            return 6;
        case 0x4D54: // MIDI
            return 7;
        case 0x4F67: // OGG
            return 8;
        default: // nothing
            return 0;
    }
}

uint16 ICACHE_FLASH_ATTR VS1053_u8ReadBitRate(void)
{
    return (VS1053_u16ReadRegister(SCI_HDAT0) << 3);
}

void ICACHE_FLASH_ATTR VS1053_vSetEnhancer(int8 Trebel_Amplitude, uint8 Trebel_Limit, uint8 Bass_Amplitude, uint8 Bass_Limit)
{
    // Directly write to volume register
    VS1053_vWriteRegister(SCI_BASS, ((Trebel_Amplitude << 12) & (0xF000)) | ((Trebel_Limit << 8) & (0x0F00)) | ((Bass_Amplitude << 4) & (0x00F0)) | ((Bass_Limit) & (0x000F)));
}

void ICACHE_FLASH_ATTR VS1053_vSetSpartialProcessing(uint8 Level)
{
    uint16 register_content;
    // Read content of SCI_MODE register
    register_content = VS1053_u16ReadRegister(SCI_MODE);
    // Change register values depending on the selected level
    switch(Level)
    {
        case 0:
            register_content &= ~(SM_EARSPEAKER_LO | SM_EARSPEAKER_HI);
            break;
        case 1:
            register_content &= ~(SM_EARSPEAKER_HI);
            register_content |= (SM_EARSPEAKER_LO);
            break;
        case 2:
            register_content &= ~(SM_EARSPEAKER_LO);
            register_content |= (SM_EARSPEAKER_HI);
            break;
        case 3:
            register_content |= (SM_EARSPEAKER_LO | SM_EARSPEAKER_HI);
            break;
        default:
            // Do nothing
            break;
    }
    // Write SCI_MODE register back
    VS1053_vWriteRegister(SCI_MODE, register_content);
    return;
}

