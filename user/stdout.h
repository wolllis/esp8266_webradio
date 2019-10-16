#ifndef STDOUT_H
#define STDOUT_H

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

int32 ICACHE_FLASH_ATTR Stdout_s32Strtol(const char *nptr, char **endptr, int base);
void ICACHE_FLASH_ATTR stdoutInit();

#endif
