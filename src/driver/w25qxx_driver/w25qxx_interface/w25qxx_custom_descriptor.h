#ifndef W25QXX_CUSTOM_DESCRIPTOR_H
#define W25QXX_CUSTOM_DESCRIPTOR_H



typedef struct customDescriptor_s
{
    void *spiInstance;          //  DRV_HANDLE sysObj.drvSPI0; timer sysObj.sysTime
    uint8_t chipSelectPin;      // = SPI_0_CS
    void *usartInstance;        // DRV_HANDLE &USART_0

} W25qxx_ASF_CustomDescriptor_s;




#endif