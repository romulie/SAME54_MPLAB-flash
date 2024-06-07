#ifndef DRV_SPI_HARMONY3_H
#define DRV_SPI_HARMONY3_H

#include "stddef.h"
#include "stdint.h"
#include "w25qxx_custom_descriptor.h"   // not used
#include "drv_spi.h"                    // Harmony3 driver

#define SPI_BAUD_RATE_HZ     10000000

#ifdef __cplusplus
 extern "C" {
#endif

/**
 * @brief     spi bus init
 * @param[in] *descr custom descriptor
 * @param[in] mode is the spi_transfer_mode
 * @return    status code
 *            - 0 success
 *            - 1 init failed
 * @note      SCLK is PB09, MOSI is PB08 MISO is PB14 and CS is PB07
 */
uint8_t spi_init(void *descr);

/**
 * @brief  spi bus deinit
 * @param[in] *descr custom descriptor
 * @return status code
 *         - 0 success
 *         - 1 deinit failed
 * @note   none
 */
uint8_t spi_deinit(void *descr);

/**
 * @brief      spi bus write read
 * @param[in]  *descr custom descriptor
 * @param[in]  *in_buf points to an input buffer
 * @param[in]  in_len is the input length
 * @param[out] *out_buf points to an output buffer
 * @param[in]  out_len is the output length
 * @return     status code
 *             - 0 success
 *             - 1 write read failed
 * @note       none
 */
uint8_t spi_write_read(void *descr, uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t out_len);


#ifdef __cplusplus
}
#endif

#endif //DRV_SPI_HARMONY3_H