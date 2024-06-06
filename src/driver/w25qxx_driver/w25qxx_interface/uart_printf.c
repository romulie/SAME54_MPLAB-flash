#include "uart_printf.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


/**
 * @brief     print format data
 * @param[in] fmt is the format data
 * @return    0
 * @note      none
 */
uint16_t uart_print(const char *const fmt, ...)
{

    char str[256];
    uint16_t len;
    va_list args;

    /* print to the buffer */
    memset((char *)str, 0, sizeof(char) * 256);
    va_start(args, fmt);
    vsnprintf((char *)str, 255, (char const *)fmt, args);
    va_end(args);

    /* send the data */
    //len = strlen((char *)str);
    len = printf((char *)str);

    return len;
}