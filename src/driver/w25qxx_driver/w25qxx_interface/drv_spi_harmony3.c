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
#if 0
    const DRV_SPI_PLIB_INTERFACE drvSPI0PlibAPI =
    {      
        .setup = (DRV_SPI_PLIB_SETUP)SERCOM4_SPI_TransferSetup,                             // SPI PLIB Setup       
        .writeRead = (DRV_SPI_PLIB_WRITE_READ)SERCOM4_SPI_WriteRead,                        // SPI PLIB WriteRead function
        .isTransmitterBusy = (DRV_SPI_PLIB_TRANSMITTER_IS_BUSY)SERCOM4_SPI_IsTransmitterBusy,// SPI PLIB Transfer Status function    
        .callbackRegister = (DRV_SPI_PLIB_CALLBACK_REGISTER)SERCOM4_SPI_CallbackRegister,   // SPI PLIB Callback Register
    };

    static const uint32_t SPI_DATA_BITS = 8;
    
    const DRV_SPI_INIT drvSPI0InitData =
    { 
        .spiPlib = &drvSPI0PlibAPI,                              // SPI PLIB API
        .remapDataBits = &SPI_DATA_BITS,
        .remapClockPolarity = 0,
        .remapClockPhase = 0,       
        .numClients = DRV_SPI_CLIENTS_NUMBER_IDX0,               // SPI Number of clients 
        .clientObjPool = (uintptr_t)&gDrvSPIObj[0],                        // SPI Client Objects Pool (uintptr_t)&sysObj[0], 
    };

    sysObj.drvSPI0 = DRV_SPI_Initialize(DRV_SPI_INDEX_0, (SYS_MODULE_INIT*)&drvSPI0InitData);
#endif
    if ( SYS_MODULE_OBJ_INVALID == sysObj.drvSPI0 )
    {
        printf("\n\r ===========================================SPI INIT FAILED!\n\r"); // Handle error
    }
    
    //printf("\n\r ===========================================SPI INIT OK!\n\r");

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
    // TODO: extract SYS_MODULE_INDEX for SPI
    //printf("\n\r About to start spi_write_read().....\n\r");
    SYS_STATUS sys_status = DRV_SPI_Status( sysObj.drvSPI0 );
    //if (SYS_STATUS_READY == sys_status )            printf("SPI driver is ready!\r\n");
    //if (SYS_STATUS_UNINITIALIZED == sys_status )    printf("SPI driver is not ready!\r\n");

    const SYS_MODULE_INDEX SPIindex = DRV_SPI_INDEX_0;
    const DRV_IO_INTENT ioIntent = DRV_IO_INTENT_EXCLUSIVE;
    DRV_HANDLE SPIhandle;
    SPIhandle = DRV_SPI_Open(SPIindex, ioIntent); // 
    if (DRV_HANDLE_INVALID == SPIhandle)
    {
        // Handle error
        //printf("\n\r =========================================== DRV_SPI_Open() Error!\n\r");
        return 1;           // could not open driver / driver is not initialized 
    }
    //else printf("\n\r =========================================== DRV_SPI_Open() OK!\n\r");
    
    //TODO: Setup current transfer
    DRV_SPI_TRANSFER_SETUP SPIsetup = 
    {
        .baudRateInHz = 50000,//SERCOM4_SPIM_BAUD_VALUE
        .clockPhase = DRV_SPI_CLOCK_PHASE_VALID_LEADING_EDGE,
        .clockPolarity = DRV_SPI_CLOCK_POLARITY_IDLE_LOW,
        .dataBits = DRV_SPI_DATA_BITS_8,
        .chipSelect = 0, //SYS_PORT_PIN_PB14
        .csPolarity = DRV_SPI_CS_POLARITY_ACTIVE_LOW,
    };
    
    //DRV_SPI_TransferSetup ( SPIhandle, &SPIsetup ); // this does not work!!
    
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
    
    DRV_SPI_Close(SPIhandle);   // Close the driver

    return 0;
}
