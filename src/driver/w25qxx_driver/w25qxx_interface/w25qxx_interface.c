#include "driver_w25qxx_interface.h"
#include "uart_printf.h"


#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "drv_spi_harmony3.h"
#include "sys_time.h"
#include "debug.h"

/**
 * @brief  interface spi qspi bus init
 * @param[in] descr - custom descriptor
 * @return status code
 *         - 0 success
 *         - 1 spi qspi init failed
 * @note   none
 */
uint8_t w25qxx_interface_spi_qspi_init(void* descr)
{
    spi_init(descr);
    
    return 0;
}

/**
 * @brief  interface spi qspi bus deinit
 * @param[in] descr - custom descriptor
 * @return status code
 *         - 0 success
 *         - 1 spi qspi deinit failed
 * @note   none
 */
uint8_t w25qxx_interface_spi_qspi_deinit(void* descr)
{
    spi_deinit(descr);
    
    return 0;
}

/**
 * @brief      interface spi qspi bus write read
 * @param[in] descr - custom descriptor
 * @param[in]  instruction is the sent instruction
 * @param[in]  instruction_line is the instruction phy lines
 * @param[in]  address is the register address
 * @param[in]  address_line is the address phy lines
 * @param[in]  address_len is the address length
 * @param[in]  alternate is the register address
 * @param[in]  alternate_line is the alternate phy lines
 * @param[in]  alternate_len is the alternate length
 * @param[in]  dummy is the dummy cycle
 * @param[in]  *in_buf points to a input buffer
 * @param[in]  in_len is the input length
 * @param[out] *out_buf points to a output buffer
 * @param[in]  out_len is the output length
 * @param[in]  data_line is the data phy lines
 * @return     status code
 *             - 0 success
 *             - 1 write read failed
 * @note       none
 */
uint8_t w25qxx_interface_spi_qspi_write_read(void *descr, uint8_t instruction, uint8_t instruction_line,
                                             uint32_t address, uint8_t address_line, uint8_t address_len,
                                             uint32_t alternate, uint8_t alternate_line, uint8_t alternate_len,
                                             uint8_t dummy, uint8_t *in_buf, uint32_t in_len,
                                             uint8_t *out_buf, uint32_t out_len, uint8_t data_line)
{
    if ((instruction_line != 0) || (address_line != 0) || (alternate_line != 0) || (dummy != 0) || (data_line != 1))
    {
        return 1;
    }

    return spi_write_read(descr, in_buf, in_len, out_buf, out_len);; 
}

/**
 * @brief     interface delay ms
 * @param[in] descr - custom descriptor
 * @param[in] ms
 * @note      none
 */
void w25qxx_interface_delay_ms(void *descr, uint32_t ms)
{
    (void)descr;
  
    SYS_TIME_HANDLE timer = SYS_TIME_HANDLE_INVALID;
    if (SYS_TIME_DelayMS(ms, &timer) != SYS_TIME_SUCCESS)
    {
        TRACE_PRINTF("\r\n================DELAY ERROR=====================\r\n");
        assert(0);        // Handle error
    }
    else if (SYS_TIME_DelayIsComplete(timer) != true)
    {
        while (SYS_TIME_DelayIsComplete(timer) == false); // Wait till the delay has not expired
    }  

#if 0     
    osDelayTask(ms);
#endif
}

/**
 * @brief     interface delay us
 * @param[in] descr - custom descriptor
 * @param[in] us
 * @note      none
 */
void w25qxx_interface_delay_us(void *descr, uint32_t us)
{
    (void)descr;

    SYS_TIME_HANDLE timer = SYS_TIME_HANDLE_INVALID;
    if (SYS_TIME_DelayUS(us, &timer) != SYS_TIME_SUCCESS)
    {
        TRACE_PRINTF("\r\n================DELAY ERROR=====================\r\n");
        assert(0);        // Handle error
    }
    else if (SYS_TIME_DelayIsComplete(timer) != true)
    {
        while (SYS_TIME_DelayIsComplete(timer) == false); // Wait till the delay has not expired
    }

#if 0    
    osDelayTask(1);
#endif
}


/**
 * @brief     interface print format data
 * @param[in] descr - custom descriptor
 * @param[in] fmt is the format data
 * @note      none
 */
void w25qxx_interface_debug_print(void *descr, const char *const fmt, ...)
{
    (void)descr;

    char str[256];
    va_list args;

    memset((char *)str, 0, sizeof(char) * 256);
    va_start(args, fmt);
    vsnprintf((char *)str, 255, (char const *)fmt, args);
    va_end(args);

    //len = strlen((char *)str);
    //(void)uart_write((uint8_t *)str, len);
    (void)printf((char *)str);
}
