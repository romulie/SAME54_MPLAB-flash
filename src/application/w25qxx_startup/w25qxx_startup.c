/*
 * w25qxx_startup.c
 *
 * Created: may 2024
 *  Author: PI
 */

#include "w25qxx_startup.h"

#include <stdint.h>
#include <stddef.h>

#include "driver_w25qxx_interface.h"
#include "driver_w25qxx.h"

// TODO: PUT ASSERTS' DEFINES HERE

// static
w25qxx_handle_t w25q128_handle = {0};

W25qxx_ASF_CustomDescriptor_s w25q128_extraDescriptor =
{
    .spiInstance    = W25QXX_STARTUP_CFG_PHY_SPI_INSTANCE_PTR,
    .chipSelectPin  = W25QXX_STARTUP_CFG_PHY_CS_PIN,
    .usartInstance  = W25QXX_STARTUP_CFG_PHY_USART_INSTANCE_PTR
};



uint8_t W25qxx_Startup()
{
    uint8_t res;
    w25qxx_info_t info;
    
    /* link interface function */
    DRIVER_W25QXX_LINK_INIT(&w25q128_handle, w25qxx_handle_t);
    DRIVER_W25QXX_LINK_SPI_QSPI_INIT(&w25q128_handle, w25qxx_interface_spi_qspi_init);
    DRIVER_W25QXX_LINK_SPI_QSPI_DEINIT(&w25q128_handle, w25qxx_interface_spi_qspi_deinit);
    DRIVER_W25QXX_LINK_SPI_QSPI_WRITE_READ(&w25q128_handle, w25qxx_interface_spi_qspi_write_read);
    DRIVER_W25QXX_LINK_DELAY_MS(&w25q128_handle, w25qxx_interface_delay_ms);
    DRIVER_W25QXX_LINK_DELAY_US(&w25q128_handle, w25qxx_interface_delay_us);
    DRIVER_W25QXX_LINK_DEBUG_PRINT(&w25q128_handle, w25qxx_interface_debug_print);
    DRIVER_W25QXX_LINK_EXTRA_VOID_PTR(&w25q128_handle, (void*)&w25q128_extraDescriptor);
    
    /* get information */
    res = w25qxx_info(&info);
    if (res != 0)
    {
        w25qxx_interface_debug_print(w25q128_handle.extra, "w25qxx: get info failed.\n");
        (void)w25qxx_deinit(&w25q128_handle);
        return 1;
    }
    else
    {
        /* print chip information */
        w25qxx_interface_debug_print(w25q128_handle.extra, "w25qxx: chip is %s.\n", info.chip_name);
        w25qxx_interface_debug_print(w25q128_handle.extra, "w25qxx: manufacturer is %s.\n", info.manufacturer_name);
        w25qxx_interface_debug_print(w25q128_handle.extra, "w25qxx: interface is %s.\n", info.interface);
        w25qxx_interface_debug_print(w25q128_handle.extra, "w25qxx: driver version is %d.%d.\n\n", info.driver_version / 1000, (info.driver_version % 1000) / 100);
    }
    
    /* set chip type */
    //w25qxx_interface_debug_print(w25q128_handle.extra, "w25qxx: about to set type.\n");
    res = w25qxx_set_type(&w25q128_handle, W25QXX_STARTUP_CFG_TYPE);
    if (res != 0)
    {
        w25qxx_interface_debug_print(w25q128_handle.extra, "w25qxx: set type failed.\n");
        (void)w25qxx_deinit(&w25q128_handle);
        return 1;
    }
    
    /* set chip interface */
    //w25qxx_interface_debug_print(w25q128_handle.extra, "w25qxx: about to set interface.\n");
    res = w25qxx_set_interface(&w25q128_handle, W25QXX_STARTUP_CFG_INTERFACE);
    if (res != 0)
    {
        w25qxx_interface_debug_print(w25q128_handle.extra, "w25qxx: set interface failed.\n");
       (void)w25qxx_deinit(&w25q128_handle);
        return 1;
    }
    
    /* set dual quad spi */
    //w25qxx_interface_debug_print(w25q128_handle.extra, "w25qxx: about to set dual_quad_spi.\n");
    res = w25qxx_set_dual_quad_spi(&w25q128_handle, W25QXX_STARTUP_CFG_DUAL_QUAD_SPI);
    if (res != 0)
    {
        w25qxx_interface_debug_print(w25q128_handle.extra, "w25qxx: set dual quad spi failed.\n");
        (void)w25qxx_deinit(&w25q128_handle);
        return 1;
    }
    
    /* chip init */
    //w25qxx_interface_debug_print(w25q128_handle.extra, "w25qxx: about to init chip.\n");
    res = w25qxx_init(&w25q128_handle);
    if (res != 0)
    {
        w25qxx_interface_debug_print(w25q128_handle.extra, "w25qxx: init failed.\n");
        (void)w25qxx_deinit(&w25q128_handle);
        return 1;
    }
    else
    {
        if (W25QXX_STARTUP_CFG_TYPE >= W25Q256)
        {
            res = w25qxx_set_address_mode(&w25q128_handle, W25QXX_ADDRESS_MODE_4_BYTE);
            if (res != 0)
            {
                w25qxx_interface_debug_print(&(w25q128_handle.extra), "w25qxx: set address mode failed.\n");
                (void)w25qxx_deinit(&w25q128_handle);
                return 1;
            }
        }
    }

    return 0;
 }