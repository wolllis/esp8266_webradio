#include <esp8266.h>
#include "spi.h"
#include "spi_register.h"

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_rx_byte_order
//   Description: Setup the byte order for shifting data into buffer
//    Parameters: spi_no - SPI (0) or HSPI (1)
//				  byte_order - SPI_BYTE_ORDER_HIGH_TO_LOW (1)
//							   Data is read in starting with Bit31 and down to Bit0
//
//							   SPI_BYTE_ORDER_LOW_TO_HIGH (0)
//							   Data is read in starting with the lowest BYTE, from
//							   MSB to LSB, followed by the second lowest BYTE, from
//							   MSB to LSB, followed by the second highest BYTE, from
//							   MSB to LSB, followed by the highest BYTE, from MSB to LSB
//							   0xABCDEFGH would be read as 0xGHEFCDAB
//
////////////////////////////////////////////////////////////////////////////////
void ICACHE_FLASH_ATTR spi_rx_byte_order(uint8 spi_no, uint8 byte_order)
{
    if (spi_no > 1)
        return;
    if (byte_order)
    {
        SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_RD_BYTE_ORDER);
    }
    else
    {
        CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_RD_BYTE_ORDER);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_tx_byte_order
//   Description: Setup the byte order for shifting data out of buffer
//    Parameters: spi_no - SPI (0) or HSPI (1)
//				  byte_order - SPI_BYTE_ORDER_HIGH_TO_LOW (1)
//							   Data is sent out starting with Bit31 and down to Bit0
//
//							   SPI_BYTE_ORDER_LOW_TO_HIGH (0)
//							   Data is sent out starting with the lowest BYTE, from
//							   MSB to LSB, followed by the second lowest BYTE, from
//							   MSB to LSB, followed by the second highest BYTE, from
//							   MSB to LSB, followed by the highest BYTE, from MSB to LSB
//							   0xABCDEFGH would be sent as 0xGHEFCDAB
//
////////////////////////////////////////////////////////////////////////////////
void ICACHE_FLASH_ATTR spi_tx_byte_order(uint8 spi_no, uint8 byte_order)
{
    if (spi_no > 1)
        return;
    if (byte_order)
    {
        SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_WR_BYTE_ORDER);
    }
    else
    {
        CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_WR_BYTE_ORDER);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_clock
//   Description: sets up the control registers for the SPI clock
//    Parameters: spi_no - SPI (0) or HSPI (1)
//				  prediv - predivider value (actual division value)
//				  cntdiv - postdivider value (actual division value)
//				  Set either divider to 0 to disable all division (80MHz sysclock)
//
////////////////////////////////////////////////////////////////////////////////
void ICACHE_FLASH_ATTR spi_clock(uint8 spi_no, uint16 prediv, uint8 cntdiv)
{
    if (spi_no > 1)
        return;
    if ((prediv == 0) | (cntdiv == 0))
    {
        WRITE_PERI_REG(SPI_CLOCK(spi_no), SPI_CLK_EQU_SYSCLK);
    }
    else
    {
        WRITE_PERI_REG(SPI_CLOCK(spi_no), (((prediv-1)&SPI_CLKDIV_PRE)<<SPI_CLKDIV_PRE_S)| (((cntdiv-1)&SPI_CLKCNT_N)<<SPI_CLKCNT_N_S)| (((cntdiv>>1)&SPI_CLKCNT_H)<<SPI_CLKCNT_H_S)| ((0&SPI_CLKCNT_L)<<SPI_CLKCNT_L_S));
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_init_gpio
//   Description: Initialises the GPIO pins for use as SPI pins.
//    Parameters: spi_no - SPI (0) or HSPI (1)
//				  sysclk_as_spiclk - SPI_CLK_80MHZ_NODIV (1) if using 80MHz
//									 sysclock for SPI clock.
//									 SPI_CLK_USE_DIV (0) if using divider to
//									 get lower SPI clock speed.
//
////////////////////////////////////////////////////////////////////////////////
void ICACHE_FLASH_ATTR spi_init_gpio(uint8 spi_no, uint8 sysclk_as_spiclk)
{
    if (spi_no > 1)
        return;
    uint32 clock_div_flag = 0;
    if (sysclk_as_spiclk)
    {
        clock_div_flag = 0x1000;
    }

    if (spi_no == SPI)
    {
        WRITE_PERI_REG(PERIPHS_IO_MUX, 0x005 | (clock_div_flag << 8)); //Set bit 8 if 80MHz sysclock required
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CLK_U, FUNC_SPICLK); //configure io to spi mode
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CMD_U, FUNC_SPICS0); //configure io to spi mode
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA0_U, FUNC_SPIQ); //configure io to spi mode
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA1_U, FUNC_SPID); //configure io to spi mode

    }
    else
        if (spi_no == HSPI)
        {
            WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105 | (clock_div_flag << 9)); //Set bit 9 if 80MHz sysclock required
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_SPI_HSPIQ_MISO); //configure io to Hspi mode
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_SPI_HSPID_MOSI); //configure io to Hspi mode
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_SPI_HSPICLK_SCK); //configure io to Hspi mode
            //PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_SPI_HPSICS_CS); //configure io to Hspi mode
        }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_init
//   Description: Wrapper to setup HSPI/SPI GPIO pins and default SPI clock
//    Parameters: spi_no - SPI (0) or HSPI (1)
//
////////////////////////////////////////////////////////////////////////////////
void ICACHE_FLASH_ATTR spi_init(uint8 spi_no)
{
    if (spi_no > 1)
        return; //Only SPI and HSPI are valid spi modules.
    spi_init_gpio(spi_no, SPI_CLK_USE_DIV);
    spi_clock(spi_no, SPI_CLK_PREDIV, SPI_CLK_CNTDIV);
    spi_tx_byte_order(spi_no, SPI_BYTE_ORDER_HIGH_TO_LOW);
    spi_rx_byte_order(spi_no, SPI_BYTE_ORDER_HIGH_TO_LOW);
    SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_CS_SETUP|SPI_CS_HOLD);
    CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_FLASH_MODE);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_transaction
//   Description: SPI transaction function
//    Parameters: spi_no - SPI (0) or HSPI (1)
//				  cmd_bits - actual number of bits to transmit
//				  cmd_data - command data
//				  addr_bits - actual number of bits to transmit
//				  addr_data - address data
//				  dout_bits - actual number of bits to transmit
//				  dout_data - output data
//				  din_bits - actual number of bits to receive
//
//		 Returns: read data - uint32 containing read in data only if RX was set
//				  0 - something went wrong (or actual read data was 0)
//				  1 - data sent ok (or actual read data is 1)
//				  Note: all data is assumed to be stored in the lower bits of
//				  the data variables (for anything <32 bits).
//
////////////////////////////////////////////////////////////////////////////////
uint32 ICACHE_FLASH_ATTR spi_transaction(uint8 spi_no, uint8 cmd_bits, uint16 cmd_data, uint32 addr_bits, uint32 addr_data, uint32 dout_bits, uint32 dout_data, uint32 din_bits, uint32 dummy_bits)
{
    if (spi_no > 1)
        return 0; //Check for a valid SPI

    // Insert code for custom Chip Select here

    while (spi_busy(spi_no)); //wait for SPI to be ready

//########## Enable SPI Functions ##########//
    //disable MOSI, MISO, ADDR, COMMAND, DUMMY in case previously set.
    CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MOSI|SPI_USR_MISO|SPI_USR_COMMAND|SPI_USR_ADDR|SPI_USR_DUMMY);

    //enable functions based on number of bits. 0 bits = disabled.
    //This is rather inefficient but allows for a very generic function.
    if (din_bits)
    {
        SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MISO);
    }
    if (dummy_bits)
    {
        SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_DUMMY);
    }

//########## Setup Bitlengths ##########//
    WRITE_PERI_REG(SPI_USER1(spi_no), ((addr_bits-1)&SPI_USR_ADDR_BITLEN)<<SPI_USR_ADDR_BITLEN_S |//Number of bits in Address
    ((dout_bits-1)&SPI_USR_MOSI_BITLEN)<<SPI_USR_MOSI_BITLEN_S |//Number of bits to Send
    ((din_bits-1)&SPI_USR_MISO_BITLEN)<<SPI_USR_MISO_BITLEN_S |//Number of bits to receive
    ((dummy_bits-1)&SPI_USR_DUMMY_CYCLELEN)<<SPI_USR_DUMMY_CYCLELEN_S);//Number of Dummy bits to insert

//########## Setup Command Data ##########//
    if (cmd_bits)
    {
        SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_COMMAND); //enable COMMAND function in SPI module
        uint16 command = cmd_data << (16 - cmd_bits); //align command data to high bits
        command = ((command >> 8) & 0xff) | ((command << 8) & 0xff00); //swap byte order
        WRITE_PERI_REG(SPI_USER2(spi_no), ((((cmd_bits-1)&SPI_USR_COMMAND_BITLEN)<<SPI_USR_COMMAND_BITLEN_S) | (command&SPI_USR_COMMAND_VALUE)));
    }
//########## Setup Address Data ##########//
    if (addr_bits)
    {
        SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_ADDR); //enable ADDRess function in SPI module
        WRITE_PERI_REG(SPI_ADDR(spi_no), addr_data << (32 - addr_bits)); //align address data to high bits
    }
//########## Setup DOUT data ##########//
    if (dout_bits)
    {
        SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MOSI); //enable MOSI function in SPI module
        //copy data to W0
        if (READ_PERI_REG(SPI_USER(spi_no)) & SPI_WR_BYTE_ORDER)
        {
            WRITE_PERI_REG(SPI_W0(spi_no), dout_data << (32 - dout_bits));
        }
        else
        {

            uint8 dout_extra_bits = dout_bits % 8;

            if (dout_extra_bits)
            {
                //if your data isn't a byte multiple (8/16/24/32 bits)and you don't have SPI_WR_BYTE_ORDER set, you need this to move the non-8bit remainder to the MSBs
                //not sure if there's even a use case for this, but it's here if you need it...
                //for example, 0xDA4 12 bits without SPI_WR_BYTE_ORDER would usually be output as if it were 0x0DA4,
                //of which 0xA4, and then 0x0 would be shifted out (first 8 bits of low byte, then 4 MSB bits of high byte - ie reverse byte order).
                //The code below shifts it out as 0xA4 followed by 0xD as you might require.
                WRITE_PERI_REG(SPI_W0(spi_no), (((0xFFFFFFFF << (dout_bits - dout_extra_bits) & dout_data) << (8 - dout_extra_bits)) | ((0xFFFFFFFF >> (32 - (dout_bits - dout_extra_bits))) & dout_data)));
            }
            else
            {
                WRITE_PERI_REG(SPI_W0(spi_no), dout_data);
            }
        }
    }
//########## Begin SPI Transaction ##########//
    SET_PERI_REG_MASK(SPI_CMD(spi_no), SPI_USR);
//########## Return DIN data ##########//
    if (din_bits)
    {
        while (spi_busy(spi_no)); //wait for SPI transaction to complete

        if (READ_PERI_REG(SPI_USER(spi_no)) & SPI_RD_BYTE_ORDER)
        {
            return READ_PERI_REG(SPI_W0(spi_no)) >> (32 - din_bits); //Assuming data in is written to MSB. TBC
        }
        else
        {
            return READ_PERI_REG(SPI_W0(spi_no)); //Read in the same way as DOUT is sent. Note existing contents of SPI_W0 remain unless overwritten!
        }
        return 0; //something went wrong
    }

    //Transaction completed
    return 1; //success
}

/*
 * Send a single byte by spi interface and return a ansver in duplex mode
 */

uint32 ICACHE_FLASH_ATTR spiwrite_8(uint32 c)
{
    //waiting for spi module to be available
    while (READ_PERI_REG(SPI_CMD(HSPI)) & SPI_USR);
    // Clear configuration of SPI_USER register
    CLEAR_PERI_REG_MASK(SPI_USER(HSPI), SPI_USR_MOSI|SPI_USR_MISO|SPI_USR_COMMAND|SPI_USR_ADDR|SPI_USR_DUMMY|SPI_USR_DOUTDIN);
    // Set SPI_USER config (MOSI + reception on MISO PIN)
    SET_PERI_REG_MASK(SPI_USER(HSPI), SPI_USR_MOSI);
    SET_PERI_REG_MASK(SPI_USER(HSPI), SPI_USR_DOUTDIN);
    // Set Data length
    WRITE_PERI_REG(SPI_USER1(HSPI), (7 & SPI_USR_MOSI_BITLEN) << SPI_USR_MOSI_BITLEN_S);
    // Write data to be transferred
    WRITE_PERI_REG(SPI_W0(HSPI), c << (32 - 8));
    // Trigger sending
    SET_PERI_REG_MASK(SPI_CMD(HSPI), SPI_USR);
    // wait for data to be send
    while (READ_PERI_REG(SPI_CMD(HSPI)) & SPI_USR);
    // return received data
    return READ_PERI_REG(SPI_W0(HSPI)) >> (32 - 8);
}

uint32 ICACHE_FLASH_ATTR spiwrite_16(uint32 c)
{
    //waiting for spi module to be available
    while (READ_PERI_REG(SPI_CMD(HSPI)) & SPI_USR);
    // Clear configuration of SPI_USER register
    CLEAR_PERI_REG_MASK(SPI_USER(HSPI), SPI_USR_MOSI|SPI_USR_MISO|SPI_USR_COMMAND|SPI_USR_ADDR|SPI_USR_DUMMY|SPI_USR_DOUTDIN);
    // Set SPI_USER config (MOSI + reception on MISO PIN)
    SET_PERI_REG_MASK(SPI_USER(HSPI), SPI_USR_MOSI);
    SET_PERI_REG_MASK(SPI_USER(HSPI), SPI_USR_DOUTDIN);
    // Set Data length
    WRITE_PERI_REG(SPI_USER1(HSPI), (15 & SPI_USR_MOSI_BITLEN) << SPI_USR_MOSI_BITLEN_S);
    // Write data to be transferred
    WRITE_PERI_REG(SPI_W0(HSPI), c << (32 - 16));
    // Trigger sending
    SET_PERI_REG_MASK(SPI_CMD(HSPI), SPI_USR);
    // wait for data to be send
    while (READ_PERI_REG(SPI_CMD(HSPI)) & SPI_USR);
    // return received data
    return READ_PERI_REG(SPI_W0(HSPI)) >> (32 - 16);
}

uint32 ICACHE_FLASH_ATTR spiwrite_32(uint32 c)
{
    //waiting for spi module to be available
    while (READ_PERI_REG(SPI_CMD(HSPI)) & SPI_USR);
    // Clear configuration of SPI_USER register
    CLEAR_PERI_REG_MASK(SPI_USER(HSPI), SPI_USR_MOSI|SPI_USR_MISO|SPI_USR_COMMAND|SPI_USR_ADDR|SPI_USR_DUMMY|SPI_USR_DOUTDIN);
    // Set SPI_USER config (MOSI + reception on MISO PIN)
    SET_PERI_REG_MASK(SPI_USER(HSPI), SPI_USR_MOSI);
    SET_PERI_REG_MASK(SPI_USER(HSPI), SPI_USR_DOUTDIN);
    // Set Data length
    WRITE_PERI_REG(SPI_USER1(HSPI), (31 & SPI_USR_MOSI_BITLEN) << SPI_USR_MOSI_BITLEN_S);
    // Write data to be transferred
    WRITE_PERI_REG(SPI_W0(HSPI), c << (32 - 32));
    // Trigger sending
    SET_PERI_REG_MASK(SPI_CMD(HSPI), SPI_USR);
    // wait for data to be send
    while (READ_PERI_REG(SPI_CMD(HSPI)) & SPI_USR);
    // return received data
    return READ_PERI_REG(SPI_W0(HSPI)) >> (32 - 32);
}
