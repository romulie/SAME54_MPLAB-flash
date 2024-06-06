/*
 * w25qxx_startup_cfg.h
 *
 * Created: may 2024
 * Author: PI
 */ 


#ifndef W25QXX_STARTUP_CFG_H_
#define W25QXX_STARTUP_CFG_H_


#include "driver_w25qxx_interface.h"



#define W25QXX_STARTUP_CFG_TYPE                         W25Q128
#define W25QXX_STARTUP_CFG_INTERFACE                    W25QXX_INTERFACE_SPI
#define W25QXX_STARTUP_CFG_DUAL_QUAD_SPI                W25QXX_BOOL_FALSE
#define W25QXX_STARTUP_CFG_PHY_SPI_INSTANCE_PTR         NULL
#define W25QXX_STARTUP_CFG_PHY_CS_PIN                   0
#define W25QXX_STARTUP_CFG_PHY_USART_INSTANCE_PTR       NULL


#endif /* W25QXX_STARTUP_CFG_H_ */