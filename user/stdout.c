/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <esp8266.h>
#include <uart.h>
#include <uart_register.h>
#include "limits.h"


static int ICACHE_FLASH_ATTR Stdout_s32IsUpper(char c)
{
    return (c >= 'A' && c <= 'Z');
}

static int ICACHE_FLASH_ATTR Stdout_s32IsAlpha(char c)
{
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

static int ICACHE_FLASH_ATTR Stdout_s32IsDigit(char c)
{
    return (c >= '0' && c <= '9');
}

int32 ICACHE_FLASH_ATTR Stdout_s32Strtol(const char *nptr, char **endptr, int base)
{
    const char *s = nptr;
    unsigned long acc;
    int c;
    unsigned long cutoff;
    int neg = 0, any, cutlim;

    /*
     * Skip white space and pick up leading +/- sign if any.
     * If base is 0, allow 0x for hex and 0 for octal, else
     * assume decimal; if base is already 16, allow 0x.
     */
    do
    {
        c = *s++;
    } while (Stdout_s32IsAlpha(c));

    if (c == '-')
    {
        neg = 1;
        c = *s++;
    }
    else
        if (c == '+')
        {
            c = *s++;
        }

    if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X'))
    {
        c = s[1];
        s += 2;
        base = 16;
    }
    else
        if ((base == 0 || base == 2) && c == '0' && (*s == 'b' || *s == 'B'))
        {
            c = s[1];
            s += 2;
            base = 2;
        }
    if (base == 0)
    {
        base = c == '0' ? 8 : 10;
    }
    /*
     * Compute the cutoff value between legal numbers and illegal
     * numbers.  That is the largest legal value, divided by the
     * base.  An input number that is greater than this value, if
     * followed by a legal input character, is too big.  One that
     * is equal to this value may be valid or not; the limit
     * between valid and invalid numbers is then based on the last
     * digit.  For instance, if the range for longs is
     * [-2147483648..2147483647] and the input base is 10,
     * cutoff will be set to 214748364 and cutlim to either
     * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
     * a value > 214748364, or equal but the next digit is > 7 (or 8),
     * the number is too big, and we will return a range error.
     *
     * Set any if any `digits' consumed; make it negative to indicate
     * overflow.
     */
    cutoff = neg ? -(unsigned long) LONG_MIN : LONG_MAX;
    cutlim = cutoff % (unsigned long) base;
    cutoff /= (unsigned long) base;
    for (acc = 0, any = 0;; c = *s++)
    {
        if (Stdout_s32IsDigit(c))
            c -= '0';
        else
            if (Stdout_s32IsAlpha(c))
                c -= Stdout_s32IsUpper(c) ? 'A' - 10 : 'a' - 10;
            else
                break;
        if (c >= base)
            break;
        if (any < 0 || acc > cutoff || acc == cutoff && c > cutlim)
            any = -1;
        else
        {
            any = 1;
            acc *= base;
            acc += c;
        }
    }
    if (any < 0)
    {
        acc = neg ? LONG_MIN : LONG_MAX;
//      errno = ERANGE;
    }
    else
        if (neg)
            acc = -acc;
    if (endptr != 0)
        *endptr = (char*) (any ? s - 1 : nptr);
    return (acc);
}

static void ICACHE_FLASH_ATTR stdoutUartTxd(char c)
{
    //Wait until there is room in the FIFO
    while (((READ_PERI_REG(UART_STATUS(0)) >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT) >= 126);
    //Send the character
    WRITE_PERI_REG(UART_FIFO(0), c);
}

static void ICACHE_FLASH_ATTR stdoutPutchar(char c)
{
    //convert \n -> \r\n
    if (c == '\n')
        stdoutUartTxd('\r');
    stdoutUartTxd(c);
}

void ICACHE_FLASH_ATTR stdoutInit()
{
    //Enable TxD pin
    PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);

    //Set baud rate and other serial parameters to 115200,n,8,1
    uart_div_modify(0, UART_CLK_FREQ / BIT_RATE_115200);
    WRITE_PERI_REG(UART_CONF0(0), (STICK_PARITY_DIS)|(ONE_STOP_BIT << UART_STOP_BIT_NUM_S)| (EIGHT_BITS << UART_BIT_NUM_S));

    //Reset tx & rx fifo
    SET_PERI_REG_MASK(UART_CONF0(0), UART_RXFIFO_RST|UART_TXFIFO_RST);
    CLEAR_PERI_REG_MASK(UART_CONF0(0), UART_RXFIFO_RST|UART_TXFIFO_RST);
    //Clear pending interrupts
    WRITE_PERI_REG(UART_INT_CLR(0), 0xffff);

    //Install our own putchar handler
    os_install_putc1((void*) stdoutPutchar);
}
