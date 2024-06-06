#ifndef UART_PRINTF_H
#define UART_PRINTF_H

#include "stdint.h"

#ifdef __cplusplus
extern "C"{
#endif

uint16_t uart_print(const char *const fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // UART_PRINTF_H
