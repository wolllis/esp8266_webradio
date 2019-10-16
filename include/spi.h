#ifndef SPI_APP_H
#define SPI_APP_H

//Define SPI hardware modules
#define SPI 0
#define HSPI 1
#define SPI_USED  HSPI

#define SPI_CLK_USE_DIV 0
#define SPI_CLK_80MHZ_NODIV 1

#define SPI_BYTE_ORDER_HIGH_TO_LOW 1
#define SPI_BYTE_ORDER_LOW_TO_HIGH 0

#define CPU_CLK_FREQ 80*1000000

//Define some default SPI clock settings
#define SPI_CLK_PREDIV 40
#define SPI_CLK_CNTDIV 2
#define SPI_CLK_FREQ CPU_CLK_FREQ/(SPI_CLK_PREDIV*SPI_CLK_CNTDIV) // 80MHz/(2*10) = 4MHz

/* Expansion macros for easy access */
#define spi_busy(spi_no) READ_PERI_REG(SPI_CMD(spi_no))&SPI_USR

#define spi_txd(spi_no, bits, data) spi_transaction(spi_no, 0, 0, 0, 0, bits, (uint32) data, 0, 0)
#define spi_tx8(spi_no, data)       spi_transaction(spi_no, 0, 0, 0, 0, 8,    (uint32) data, 0, 0)
#define spi_tx16(spi_no, data)      spi_transaction(spi_no, 0, 0, 0, 0, 16,   (uint32) data, 0, 0)
#define spi_tx32(spi_no, data)      spi_transaction(spi_no, 0, 0, 0, 0, 32,   (uint32) data, 0, 0)

#define spi_rxd(spi_no, bits) spi_transaction(spi_no, 0, 0, 0, 0, 0, 0, bits, 0)
#define spi_rx8(spi_no)       spi_transaction(spi_no, 0, 0, 0, 0, 0, 0, 8,    0)
#define spi_rx16(spi_no)      spi_transaction(spi_no, 0, 0, 0, 0, 0, 0, 16,   0)
#define spi_rx32(spi_no)      spi_transaction(spi_no, 0, 0, 0, 0, 0, 0, 32,   0)

// Missing IO_MUX definitions
#define FUNC_SPI_HSPIQ_MISO		2
#define FUNC_SPI_HSPID_MOSI 	2
#define FUNC_SPI_HSPICLK_SCK 	2
#define FUNC_SPI_HPSICS_CS 		2

void ICACHE_FLASH_ATTR spi_rx_byte_order(uint8 spi_no, uint8 byte_order);
void ICACHE_FLASH_ATTR spi_tx_byte_order(uint8 spi_no, uint8 byte_order);
void ICACHE_FLASH_ATTR spi_clock(uint8 spi_no, uint16 prediv, uint8 cntdiv);
void ICACHE_FLASH_ATTR spi_init_gpio(uint8 spi_no, uint8 sysclk_as_spiclk);
void ICACHE_FLASH_ATTR spi_init(uint8 spi_no);
uint32 ICACHE_FLASH_ATTR spi_transaction(uint8 spi_no, uint8 cmd_bits, uint16 cmd_data, uint32 addr_bits, uint32 addr_data, uint32 dout_bits, uint32 dout_data, uint32 din_bits, uint32 dummy_bits);
uint32 ICACHE_FLASH_ATTR spiwrite_8(uint32 c);
uint32 ICACHE_FLASH_ATTR spiwrite_16(uint32 c);
uint32 ICACHE_FLASH_ATTR spiwrite_32(uint32 c);

#endif
