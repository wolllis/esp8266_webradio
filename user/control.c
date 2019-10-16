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
#include "control.h"
#include "cgiwebsocket.h"
#include "vs1053.h"
#include "espfs.h"

#define BKP_ReadBackupRegister(x) Control_tstBackupDataRegister.x
#define BKP_WriteBackupRegister(x, y) Control_tstBackupDataRegister.x = y

#define PREBACKUPDATAREGISTERACTION() \
        spi_flash_read(ESP_SPI_FLASH_LAST_PAGE * ESP_SPI_FLASH_PAGE_SIZE, (uint32*) &Control_tstBackupDataRegister, sizeof(Control_tstBackupDataRegister));

#define POSTBACKUPDATAREGISTERACTION() \
        spi_flash_erase_sector(ESP_SPI_FLASH_LAST_PAGE); \
        spi_flash_write(ESP_SPI_FLASH_LAST_PAGE * ESP_SPI_FLASH_PAGE_SIZE, (uint32*) &Control_tstBackupDataRegister, sizeof(Control_tstBackupDataRegister));

struct Control_stBackupDataRegister
{
    /* Needs to be byte aligned by 4!! */
    char BKP_DR0; // Version
    char BKP_DR1; // Magic byte
    char BKP_DR2; // Volume_Left
    char BKP_DR3; // Volume_Right
    char BKP_DR4; // Treble_Amplification
    char BKP_DR5; // Treble_Limit
    char BKP_DR6; // Bass_Amplification
    char BKP_DR7; // Bass_Limit
    char BKP_DR8; // Auto Start
    char BKP_DR9; // Spartial Processing Level
    char BKP_DR10;
    char BKP_DR11;
/* Needs to be byte aligned by 4!! */
} Control_tstBackupDataRegister;

Control_tstEnhancerSettings Control_stEnhancerData = { 0, 3, 0, 10 };
uint8_t Control_u8VolumeLeft = 0;
uint8_t Control_u8VolumeRight = 0;
uint8_t Control_u8AutoStart = 0;
Control_tenSpartialProcessing Control_enSpartialProcessingLevel = Control_enSpartialProcessing_Off;

void ICACHE_FLASH_ATTR Control_vInit(void)
{
    // Load SPI flash content
    PREBACKUPDATAREGISTERACTION();

    // Check for magic byte in data register 1
    if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5)
    {
        // Data not valid (or has not been written yet) => Get default settings
        Control_u8VolumeLeft = 0;
        Control_u8VolumeRight = 0;
        Control_stEnhancerData.TrebleAmp = 0;
        Control_stEnhancerData.TrebleLim = 3;
        Control_stEnhancerData.BassAmp = 0;
        Control_stEnhancerData.BassLim = 10;
        Control_u8AutoStart = 0;
        Control_enSpartialProcessingLevel = Control_enSpartialProcessing_Off;

        // Save default data to backup register
        BKP_WriteBackupRegister(BKP_DR2, Control_u8VolumeLeft);
        BKP_WriteBackupRegister(BKP_DR3, Control_u8VolumeRight);
        BKP_WriteBackupRegister(BKP_DR4, Control_stEnhancerData.TrebleAmp);
        BKP_WriteBackupRegister(BKP_DR5, Control_stEnhancerData.TrebleLim);
        BKP_WriteBackupRegister(BKP_DR6, Control_stEnhancerData.BassAmp);
        BKP_WriteBackupRegister(BKP_DR7, Control_stEnhancerData.BassLim);
        BKP_WriteBackupRegister(BKP_DR8, Control_u8AutoStart);
        BKP_WriteBackupRegister(BKP_DR9, Control_enSpartialProcessingLevel);

        // Set backup data register version (used for compatibility with future versions)
        BKP_WriteBackupRegister(BKP_DR0, 0x00);
        // Write magic byte to data register 1
        BKP_WriteBackupRegister(BKP_DR1, 0xA5);
    }
    else
    {
        // #### For future use - In case of new data entrys this can be used ####
        //switch (BKP_ReadBackupRegister(BKP_DR0))
        //{
        //    case 0:
        //        // We will write new data register now
        //        BKP_WriteBackupRegister(BKP_DR24, 10);
        //        // Increment Software Version register
        //        BKP_WriteBackupRegister(BKP_DR0, 0x01);
        //    default:
        //        // do nothing
        //        break;
        //}

        // Data is valid => Load register values to RAM
        Control_u8VolumeLeft = BKP_ReadBackupRegister(BKP_DR2);
        Control_u8VolumeRight = BKP_ReadBackupRegister(BKP_DR3);
        Control_stEnhancerData.TrebleAmp = BKP_ReadBackupRegister(BKP_DR4);
        Control_stEnhancerData.TrebleLim = BKP_ReadBackupRegister(BKP_DR5);
        Control_stEnhancerData.BassAmp = BKP_ReadBackupRegister(BKP_DR6);
        Control_stEnhancerData.BassLim = BKP_ReadBackupRegister(BKP_DR7);
        Control_u8AutoStart = BKP_ReadBackupRegister(BKP_DR8);
        Control_enSpartialProcessingLevel = BKP_ReadBackupRegister(BKP_DR9);
    }

    // Save SPI flash content
    POSTBACKUPDATAREGISTERACTION();
}

void ICACHE_FLASH_ATTR Control_vSetVolume(uint8 value)
{
    uint8 temp;
    // Store value
    Control_u8VolumeLeft = value;
    Control_u8VolumeRight = value;
    // Calculate volume
    temp = 0xFF - ((Control_u8VolumeLeft * 5) / 2);
    // Send value to VS1053
    VS1053_vSetVolume(temp, temp);
    // Save to persistent memory
    PREBACKUPDATAREGISTERACTION();
    BKP_WriteBackupRegister(BKP_DR2, value);
    POSTBACKUPDATAREGISTERACTION();
}

uint8 ICACHE_FLASH_ATTR Control_u8GetVolume(void)
{
    // Return value
    return Control_u8VolumeLeft;
}

void ICACHE_FLASH_ATTR Control_vSetEnhancer(Control_tstEnhancerSettings *data)
{
    // Store value
    Control_stEnhancerData.TrebleAmp = data->TrebleAmp;
    Control_stEnhancerData.TrebleLim = data->TrebleLim;
    Control_stEnhancerData.BassAmp = data->BassAmp;
    Control_stEnhancerData.BassLim = data->BassLim;
    // Send value to VS1053
    VS1053_vSetEnhancer(Control_stEnhancerData.TrebleAmp, Control_stEnhancerData.TrebleLim, Control_stEnhancerData.BassAmp, Control_stEnhancerData.BassLim);
    // Save to persistent memory
    PREBACKUPDATAREGISTERACTION();
    BKP_WriteBackupRegister(BKP_DR4, Control_stEnhancerData.TrebleAmp);
    BKP_WriteBackupRegister(BKP_DR5, Control_stEnhancerData.TrebleLim);
    BKP_WriteBackupRegister(BKP_DR6, Control_stEnhancerData.BassAmp);
    BKP_WriteBackupRegister(BKP_DR7, Control_stEnhancerData.BassLim);
    POSTBACKUPDATAREGISTERACTION();
}

void ICACHE_FLASH_ATTR Control_vGetEnhancer(Control_tstEnhancerSettings *data)
{
    // Store value
    data->TrebleAmp = Control_stEnhancerData.TrebleAmp;
    data->TrebleLim = Control_stEnhancerData.TrebleLim;
    data->BassAmp = Control_stEnhancerData.BassAmp;
    data->BassLim = Control_stEnhancerData.BassLim;
    return;
}

void ICACHE_FLASH_ATTR Control_vSetAutoStart(uint8 value)
{
    // Store value
    Control_u8AutoStart = value;
    // Save to persistent memory
    PREBACKUPDATAREGISTERACTION();
    BKP_WriteBackupRegister(BKP_DR8, value);
    POSTBACKUPDATAREGISTERACTION();
}

uint8 ICACHE_FLASH_ATTR Control_u8GetAutoStart(void)
{
    // Return value
    return Control_u8AutoStart;
}

void ICACHE_FLASH_ATTR Control_vSetSpartialProcessingLevel(
    Control_tenSpartialProcessing Level)
{
    // Store value
    Control_enSpartialProcessingLevel = Level;
    // Save to persistent memory
    PREBACKUPDATAREGISTERACTION();
    BKP_WriteBackupRegister(BKP_DR9, Level);
    POSTBACKUPDATAREGISTERACTION();
}

Control_tenSpartialProcessing ICACHE_FLASH_ATTR Control_u8GetSpartialProcessingLevel(
    void)
{
    // Return value
    return Control_enSpartialProcessingLevel;
}

static char* Control_ptrReadStreamTxt(void)
{
    EspFsFile *file;
    int filesize, readsize;
    char *content;

    // Read file
    file = espFsOpen("streamlist.txt");
    // Get filesize
    filesize = espFsFilesize(file);
    // Allocate memory for file
    content = (char*) os_malloc(filesize);
    // Check for success
    if (content == NULL)
    {
        myprintf("os_malloc() returned NULL\n");
        return NULL;
    }
    // Copy file content to RAM
    readsize = espFsRead(file, content, filesize);
    // Return
    return content;
}

char stream_name[3][30];
char stream_address[3][100];

static void Control_vParseStreamTxt(char *data)
{
    char *start_pos;
    char *end_pos;
    uint32 count = 0;

    start_pos = data;

    // Iterate over file
    while (true)
    {
        // Search for first ";"
        end_pos = ets_strstr(start_pos, ";");
        // Check if any ";" has been found
        if (end_pos == NULL)
        {
            // No ";" found
            break;
        }
        else
        {
            // ";" has been found
            // Copy content to array
            os_memcpy(&stream_name[count][0], start_pos, end_pos - start_pos);
            // Terminate
            stream_name[count][end_pos - start_pos] = '\0';
        }
        // Update search parameter
        start_pos = end_pos + 1;
        // Search for first "newline"
        end_pos = ets_strstr(start_pos, "\r\n");
        // Check if any "newline" has been found
        if (end_pos == NULL) // Only one entry
        {
            // No "newline" found
            break;
        }
        else
        {
            // "newline" has been found
            // Copy content to array
            os_memcpy(&stream_address[count][0], start_pos, end_pos - start_pos);
            // Terminate
            stream_address[count][end_pos - start_pos] = '\0';
        }
        // Update search parameter
        start_pos = end_pos + 2;
        // Increment count variable
        count++;
    }
}

static void ICACHE_FLASH_ATTR Control_vGetStreamCount(void)
{
    char *content;

    // Open and read StreamList.txt
    content = Control_ptrReadStreamTxt();
    // Parse Stream List
    Control_vParseStreamTxt(content);
    // Printout SteamList content
    myprintf("Name: %s Address: %s\n", &stream_name[0][0], &stream_address[0][0]);
    myprintf("Name: %s Address: %s\n", &stream_name[1][0], &stream_address[1][0]);
    myprintf("Name: %s Address: %s\n", &stream_name[2][0], &stream_address[2][0]);
    // Free allocated memory
    os_free(content);
}

void ICACHE_FLASH_ATTR Control_v8GetStreamList(void)
{
    Control_vGetStreamCount();
}
