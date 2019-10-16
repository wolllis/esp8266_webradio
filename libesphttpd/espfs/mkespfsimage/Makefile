GZIP_COMPRESSION ?= no
USE_HEATSHRINK ?= yes

TARGET = mkespfsimage.exe

CC = gcc
LD = $(CC)
CFLAGS=-c -I../../lib/heatshrink -I.. -Imman-win32 -std=gnu99
LDFLAGS=-Lmman-win32 -lmman 

ifeq ("$(GZIP_COMPRESSION)","yes")
CFLAGS		+= -DESPFS_GZIP
endif

ifeq ("$(USE_HEATSHRINK)","yes")
CFLAGS		+= -DESPFS_HEATSHRINK
endif

OBJS=main.o heatshrink_encoder.o
TARGET=mkespfsimage

$(TARGET): $(OBJS)
ifeq ("$(GZIP_COMPRESSION)","yes")
	$(CC) -o $@ $^ -lz
else
	$(CC) -o $@ $^
endif

clean:
	rm -f $(TARGET) $(OBJS)