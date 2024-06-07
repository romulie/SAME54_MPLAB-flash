#include "drv_spi_harmony3.h"
#include "configuration.h"
#include "definitions.h"
#include "plib_sercom4_spi_master.h" 
#include <stdio.h>


//NVM_SPI (Port B)
//#define NVM_SPI_GROUP       1
//NVM_SPI_SS (PB14)
//#define NVM_SPI_SS_PIN      14
//#define NVM_SPI_SS_MASK     (1U << NVM_SPI_SS_PIN)

static DRV_HANDLE SPIhandle = { 0 };


/**
 * @brief  spi cs init
 * @param[in] *descr custom descriptor
 * @return status code
 *         - 0 success
 * @note   none
 */
static uint8_t a_spi_cs_init(void *descr)
{
   //TODO: check descr for NULL and extract SPI instance here
   //Configure NVM_SPI_SS pin (as output)
   //PORT_REGS->GROUP[NVM_SPI_GROUP].PORT_DIRSET = NVM_SPI_SS_MASK;
   //PORT_REGS->GROUP[NVM_SPI_GROUP].PORT_OUTCLR = NVM_SPI_SS_MASK;
    return 0;
}

static uint8_t a_spi_cs_low(void *descr)
{
    //TODO: check descr for NULL and extract SPI SS pin instance here
    //PORT_REGS->GROUP[NVM_SPI_GROUP].PORT_OUTCLR = NVM_SPI_SS_MASK;
    NVM_SPI_SS_Clear();
    return 0;
}

static uint8_t a_spi_cs_high(void *descr)
{
    //TODO: check descr for NULL and extract SPI SS pin instance here
    //PORT_REGS->GROUP[NVM_SPI_GROUP].PORT_OUTSET = NVM_SPI_SS_MASK;
    NVM_SPI_SS_Set();
    return 0;
}


uint8_t spi_init(void *descr)
{
    // DRV_SPI_Initialize() is called in SYS_Initialize() (main.c)

    // Get the current status of the SPI driver module. Function DRV_SPI_Initialize should have been called before calling this function.
    SYS_STATUS sys_status = DRV_SPI_Status(sysObj.drvSPI0);
    if (SYS_STATUS_UNINITIALIZED == sys_status)
    {
        printf("\n\r =========================================== SPI driver is not ready!\r\n");
    }

    // TODO: extract data from descr here
    const SYS_MODULE_INDEX SPIindex     = DRV_SPI_INDEX_0;
    const DRV_IO_INTENT ioIntent        = DRV_IO_INTENT_EXCLUSIVE;
    //DRV_HANDLE SPIhandle; global
    
    // Function DRV_SPI_Initialize must have been called before calling this function.
    SPIhandle = DRV_SPI_Open(SPIindex, ioIntent); // 
    if (DRV_HANDLE_INVALID == SPIhandle)
    {      
        printf("\n\r =========================================== DRV_SPI_Open() Error!\n\r"); // Handle error
        return 1;           // could not open driver / driver is not initialized 
    }
    
    const DRV_SPI_TRANSFER_SETUP SPIsetup = 
    {
        .baudRateInHz = 1000000,  //SERCOM4_SPIM_BAUD_VALUE
        .clockPhase = DRV_SPI_CLOCK_PHASE_VALID_LEADING_EDGE,
        .clockPolarity = DRV_SPI_CLOCK_POLARITY_IDLE_LOW,
        .dataBits = DRV_SPI_DATA_BITS_8,
        .chipSelect = 0, //SYS_PORT_PIN_PB14,
        .csPolarity = DRV_SPI_CS_POLARITY_ACTIVE_LOW,
    };
    
    // DRV_SPI_Open must have been called to obtain a valid opened device handle.
    if (!DRV_SPI_TransferSetup(SPIhandle, &SPIsetup))
    {
        printf("\n\r =========================================== DRV_SPI_TransferSetup() Error!\n\r"); // Handle error
        return 1;           
    }  
    
    DRV_SPI_Close(SPIhandle);   // Close the driver
    
    return a_spi_cs_init(descr);
}

/**
 * @brief  spi bus deinit
 * @param[in] *descr custom descriptor
 * @return status code
 *         - 0 success
 *         - 1 deinit failed
 * @note   none
 */
uint8_t spi_deinit(void *descr)
{
// TODO: ...
    DRV_SPI_Close(SPIhandle);   // Close the driver
    descr = NULL;
    return 0;
}

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
uint8_t spi_write_read(void *descr, uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t out_len)
{   
    // TODO: extract SYS_MODULE_INDEX, etc for SPI here
    static const SYS_MODULE_INDEX SPIindex = DRV_SPI_INDEX_0;
    static const DRV_IO_INTENT ioIntent = DRV_IO_INTENT_EXCLUSIVE;
    
    
    SPIhandle = DRV_SPI_Open(SPIindex, ioIntent); // 
    if (DRV_HANDLE_INVALID == SPIhandle)
    {
        printf("\n\r =========================================== DRV_SPI_Open() Error!\n\r"); // Handle error
        return 1;           // could not open driver / driver is not initialized 
    }
    
    a_spi_cs_low(descr);        // set SS low
    
    if (in_len > 0)
    {
        DRV_SPI_WriteTransfer(SPIhandle, (void *)in_buf, (size_t)in_len);
    }
    if (out_len > 0)
    {
        DRV_SPI_ReadTransfer(SPIhandle, (void *)out_buf, (size_t)out_len);
    }
    
    a_spi_cs_high(descr);       // set SS high
    
    DRV_SPI_Close(SPIhandle);  // Close the driver

    return 0;
}
