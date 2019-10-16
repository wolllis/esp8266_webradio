# MIT License
# 
# Copyright (c) 2019, wolllis
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

FIRMW_VERSION=1

# Unkomment this to replace the new firmware with itself
# It MUST be commented out for any official firmware release! Otherwise it will break the update process!
DEBUG_VERSION=1

ifeq ($(DEBUG_VERSION),1)
NEXT_FIRMW_VERSION=$(FIRMW_VERSION)
else
NEXT_FIRMW_VERSION= $(shell expr $(FIRMW_VERSION) + 1)
endif

NEXT_OTA_ROM0=0.bin
NEXT_OTA_ROM1=1.bin

OTA_ROM0=0.bin
OTA_ROM1=1.bin

#OUTPUT_TYPE=standalone
OUTPUT_TYPE=OTA_update

SDK_BASE		?= c:/Espressif/ESP8266_SDK
XTENSA_BINDIR 	?= c:/Espressif/xtensa-lx106-elf/bin/
SDK_TOOLS		?= c:/Espressif/utils/ESP8266

SDK_LIBDIR  = lib
SDK_INCDIR  = include

ESPTOOL2	?= ./tool/esptool2
ESPTOOL 	?= $(SDK_TOOLS)/esptool.exe

FW_SECTS      = .text .data .rodata
FW_USER_ARGS  = -quiet -bin -boot2 -iromchksum 

ESPPORT = COM12
ESPDELAY	?= 3 #indicates seconds to wait between flashing the two binary images
ESPBAUD		?= 1500000

ESP_SPI_FLASH_SIZE_K=4096 # SPI flash size, in KB => 4MB
ESP_SPI_FLASH_PAGE_SIZE=4096 # 4096 Byte per page
ESP_SPI_FLASH_LAST_PAGE=1018 # Last user page available before system parameter area

ESP_FLASH_MODE=0 #0: QIO, 1: QOUT, 2: DIO, 3: DOUT
ESP_FLASH_FREQ_DIV=0 #0: 40MHz, 1: 26MHz, 2: 20MHz, 15: 80MHz

define maplookup
$(patsubst $(strip $(1)):%,%,$(filter $(strip $(1)):%,$(2)))
endef

ESPTOOL_FREQ = $(call maplookup,$(ESP_FLASH_FREQ_DIV),0:40m 1:26m 2:20m 0xf:80m 15:80m)
ESPTOOL_MODE = $(call maplookup,$(ESP_FLASH_MODE),0:qio 1:qout 2:dio 3:dout)
ESPTOOL_SIZE = $(call maplookup,$(ESP_SPI_FLASH_SIZE_K),512:512KB 256:256KB 1024:1MB 2048:2MB 4096:4MB)

ESPTOOL_OPTS=--port $(ESPPORT) --baud $(ESPBAUD)
ESPTOOL_FLASHDEF=--flash_freq $(ESPTOOL_FREQ) --flash_mode $(ESPTOOL_MODE) --flash_size $(ESPTOOL_SIZE)

INITDATAPOS=$(call maplookup,$(ESPTOOL_SIZE),1MB:0xFC000 2MB:0x1FC000 4MB:0x3FC000 8MB:0x7FC000 16MB:0xFFC000)
BLANKPOS=$(call maplookup,$(ESPTOOL_SIZE),1MB:0xFE000 2MB:0x1FE000 4MB:0x3FE000 8MB:0x7FE000 16MB:0xFFE000)

CC := $(addprefix $(XTENSA_BINDIR)/,xtensa-lx106-elf-gcc)
LD := $(addprefix $(XTENSA_BINDIR)/,xtensa-lx106-elf-gcc)

BUILD_DIR = build
FIRMW_DIR = firmware

SDK_LIBDIR := $(addprefix $(SDK_BASE)/,$(SDK_LIBDIR))
SDK_INCDIR := $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))
SDK_INCDIR += -I./libesphttpd/include -I./include -I./user -I./include/driver

LIBS    = c gcc hal phy net80211 lwip wpa main pp crypto ssl
LIBS	+= esphttpd
LIBS	+= webpages-espfs
LIBS	:= $(addprefix -l,$(LIBS))

CFLAGS  = -Os -g -O2 -Wpointer-arith -Wundef -Werror -Wno-implicit -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls  -mtext-section-literals  -D__ets__ -DICACHE_FLASH
CFLAGS 	+= -DESP_SPI_FLASH_PAGE_SIZE=$(ESP_SPI_FLASH_PAGE_SIZE) 
CFLAGS  += -DESP_SPI_FLASH_LAST_PAGE=$(ESP_SPI_FLASH_LAST_PAGE)
CFLAGS  += -DOTA_ROM0=\"$(NEXT_OTA_ROM0)\"
CFLAGS  += -DOTA_ROM1=\"$(NEXT_OTA_ROM1)\"
CFLAGS  += -DFIRMW_VERSION=\"$(FIRMW_VERSION)\" 
ifeq ($(DEBUG_VERSION),1)
CFLAGS  += -DDEBUG_VERSION=\"$(DEBUG_VERSION)\"
endif

LDFLAGS = -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static -Wl,-Map, $(BUILD_DIR)/firmware.map

C_FILES = $(wildcard *.c) $(wildcard */*.c)
O_FILES = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_FILES))

.SECONDARY:
.PHONY: all clean

# What targets to created depending on the build type
ifeq ("$(OUTPUT_TYPE)","standalone")
all: libesphttpd.a $(FIRMW_DIR) $(BUILD_DIR) $(FIRMW_DIR)/standalone.bin
endif
ifeq ("$(OUTPUT_TYPE)","OTA_update")
all: libesphttpd.a $(FIRMW_DIR) $(BUILD_DIR) $(FIRMW_DIR)/Esp_$(FIRMW_VERSION)_0.bin $(FIRMW_DIR)/Esp_$(FIRMW_VERSION)_1.bin
endif

# Define how to handle object files by compiling source code files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)/$(dir $<)
	@echo "CC $(notdir $<)"
	@$(CC) -I. $(SDK_INCDIR) $(CFLAGS) -o $@ -c $<

# Define how to create elf files out of available object files
$(BUILD_DIR)/%.elf: $(O_FILES)
	@echo "LD $(notdir $@)"
	@$(LD) -L$(SDK_LIBDIR) -Llibesphttpd -T$(BUILD_DIR)/$(notdir $(basename $@)).ld $(LDFLAGS) -Wl,--start-group $(LIBS) $^ -Wl,--end-group -o $@

# Define how to create bin files
$(FIRMW_DIR)/%.bin: $(BUILD_DIR)/%.elf
	@echo "FW $(notdir $@)"
ifeq ("$(OUTPUT_TYPE)","standalone")
	@$(ESPTOOL) elf2image $< --output $(FIRMW_DIR)/
endif
ifeq ("$(OUTPUT_TYPE)","OTA_update")
	@$(ESPTOOL2) $(FW_USER_ARGS) $^ $@ $(FW_SECTS)
	@echo $(ESP_SPI_FLASH_LAST_SECTOR)
endif
	
# create output dir
firmware:
	@mkdir -p $(FIRMW_DIR)

# Create output dir and mapfile
build:
	@mkdir -p $(BUILD_DIR)
	@echo "" > $(BUILD_DIR)/firmware.map
	@cp -f tool/rom0.ld $(BUILD_DIR)/Esp_$(FIRMW_VERSION)_0.ld
	@cp -f tool/rom1.ld $(BUILD_DIR)/Esp_$(FIRMW_VERSION)_1.ld
	
clean:
	@echo "RM $(BUILD_DIR) $(FIRMW_DIR)"
	@rm -rf $(FIRMW_DIR)
	@rm -rf $(BUILD_DIR)
	@make -C libesphttpd clean

# Execute subdir makefile to build the esphttpd library
libesphttpd.a:
	@make -C libesphttpd all
	
# Flash the bootloader + first rom + second rom + init data for rfcal sectors + blank data for rf parameters
blank_flash: $(TARGET_OUT) $(FW_BASE)
ifeq ("$(OUTPUT_TYPE)","standalone")
	$(ESPTOOL) $(ESPTOOL_OPTS) write_flash $(ESPTOOL_FLASHDEF) 0x00000 $(FIRMW_DIR)/0x00000.bin 0x40000 $(FIRMW_DIR)/0x40000.bin $(BLANKPOS) $(SDK_BASE)/bin/blank.bin
endif
ifeq ("$(OUTPUT_TYPE)","OTA_update")
	$(ESPTOOL) $(ESPTOOL_OPTS) write_flash $(ESPTOOL_FLASHDEF) 0x00000 ./tool/rboot.bin 0x02000 $(FIRMW_DIR)/Esp_$(FIRMW_VERSION)_0.bin 0x82000 $(FIRMW_DIR)/Esp_$(FIRMW_VERSION)_1.bin $(INITDATAPOS) $(SDK_BASE)/bin/esp_init_data_default_v05.bin $(BLANKPOS) $(SDK_BASE)/bin/blank.bin
endif

# Flash the bootloader + first rom + second rom + init data for rfcal sectors
flash: $(TARGET_OUT) $(FW_BASE)
ifeq ("$(OUTPUT_TYPE)","standalone")
	$(ESPTOOL) $(ESPTOOL_OPTS) write_flash $(ESPTOOL_FLASHDEF) 0x00000 $(FIRMW_DIR)/0x00000.bin 0x40000 $(FIRMW_DIR)/0x40000.bin
endif
ifeq ("$(OUTPUT_TYPE)","OTA_update")
	$(ESPTOOL) $(ESPTOOL_OPTS) write_flash $(ESPTOOL_FLASHDEF) 0x00000 ./tool/rboot.bin 0x02000 $(FIRMW_DIR)/Esp_$(FIRMW_VERSION)_0.bin 0x82000 $(FIRMW_DIR)/Esp_$(FIRMW_VERSION)_1.bin $(INITDATAPOS) $(SDK_BASE)/bin/esp_init_data_default_v05.bin
endif
	
# Erase the complete external flash content (including rfcal sectors and user config data)
erase:
	$(ESPTOOL) $(ESPTOOL_OPTS) erase_flash
	
# Create a copy of the complete external spi flash memory
read:
	$(ESPTOOL) $(ESPTOOL_OPTS) read_flash 0 $$(($(ESP_SPI_FLASH_SIZE_K)*1024)) $(FIRMW_DIR)/flash.bin
	