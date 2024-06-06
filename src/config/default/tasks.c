/*******************************************************************************
 System Tasks File

  File Name:
    tasks.c

  Summary:
    This file contains source code necessary to maintain system's polled tasks.

  Description:
    This file contains source code necessary to maintain system's polled tasks.
    It implements the "SYS_Tasks" function that calls the individual "Tasks"
    functions for all polled MPLAB Harmony modules in the system.

  Remarks:
    This file requires access to the systemObjects global data structure that
    contains the object handles to all MPLAB Harmony module objects executing
    polled in the system.  These handles are passed into the individual module
    "Tasks" functions to identify the instance of the module to maintain.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *******************************************************************************/
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "configuration.h"
#include "definitions.h"
#include "sys_tasks.h"
#include "debug.h"
#include "w25qxx_startup.h"
#include "littlefs_startup.h"
#include "ftp_startup.h"
// just for tests:
#include "driver_w25qxx.h"
#include "driver_w25qxx_basic.h"
#include "driver_w25qxx_advance.h"
#include "driver_w25qxx_read_test.h"
#include "driver_w25qxx_register_test.h"


// *****************************************************************************
// *****************************************************************************
// Section: RTOS "Tasks" Routine
// *****************************************************************************
// *****************************************************************************
/* Handle for the APP_Tasks. */
TaskHandle_t xAPP_Tasks;

static void lAPP_Tasks(  void *pvParameters  )
{   
     
    /* Maintain Device Drivers */
    int err = W25qxx_Startup();
    if (err)
    {
         TRACE_INFO("\n\r----------------W25qxx_Startup() FAILED!\r\n");
    }
        else
    {
        TRACE_INFO("\n\r\++++++++++++++++W25qxx_Startup() OK!\r\n");
    }
    
    osDelayTask(500);
    
//ATTENTION: !!!LFS does not Start if tests are run before the LFS_Startup()!!!
    TRACE_INFO("\r\nAbout to start w25qxx register test!\r\n");
    w25qxx_register_test((w25qxx_type_t)W25Q128, (w25qxx_interface_t)W25QXX_INTERFACE_SPI, (w25qxx_bool_t)W25QXX_BOOL_FALSE);
    
    osDelayTask(500);
    
    //Commented-out since the test is quite long
    //TRACE_INFO("\r\nAbout to start w25qxx read test!\r\n");
    //w25qxx_read_test((w25qxx_type_t)W25Q128, (w25qxx_interface_t)W25QXX_INTERFACE_SPI, (w25qxx_bool_t)W25QXX_BOOL_FALSE);
    
    osDelayTask(500);
   
    /* Start W25qxx driver again after the tests */
    err = W25qxx_Startup();
    if (err)
    {
         TRACE_INFO("\r\n----------------W25qxx_Startup() FAILED!\r\n");
    }
        else
    {
        TRACE_INFO("\r\n++++++++++++++++W25qxx_Startup() OK!\r\n");
    }   
  
    // No need since the FS is initiated during FTP server startup (see fsInit())
    // but is needed after chip erase test
    // TODO: try fsInit() from Ftp_Startup module
    TRACE_INFO("About to start LittleFS.......\n");
    err = Littlefs_Startup();
    if (err)
    {
        TRACE_INFO("----------------LittleFS_Startup() FAILED!\r\n");
    }
    else
    {
        TRACE_INFO("++++++++++++++++LittleFS_Startup() OK!\r\n");
    }    
    
    osDelayTask(1000);
    
 
    /* Maintain Middleware & Other Libraries */
    TRACE_INFO("About to start FTP server.......\r\n");
    err = Ftp_Startup();
    if (err)
    {
        TRACE_INFO("----------------Ftp_Startup() FAILED!\r\n");
    }
    else
    {
        TRACE_INFO("++++++++++++++++Ftp_Startup() OK!\r\n");
    }
 
    
    while(true)
    {
        APP_Tasks();
        LED0_Toggle();
        //TRACE_DEBUG("Toggling the LED0.\r\n");
        osDelayTask(1000);
    }
}



// *****************************************************************************
// *****************************************************************************
// Section: System "Tasks" Routine
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void SYS_Tasks ( void )

  Remarks:
    See prototype in system/common/sys_module.h.
*/
void SYS_Tasks ( void )
{
    int err = 0;
    
    /* Maintain system services */
    TRACE_DEBUG("\nHello. This is PI_Ftp_flash_programmer.\r\nAbout to start w25qxx......\r\n\n");

    
    /* Maintain the application's state machine. */
        /* Create OS Thread for APP_Tasks. */
    (void) xTaskCreate((TaskFunction_t) lAPP_Tasks,
                "APP_Tasks",
                4*4096,
                NULL,
                1,
                &xAPP_Tasks);


    /* Start RTOS Scheduler. */
    
     /**********************************************************************
     * Create all Threads for APP Tasks before starting FreeRTOS Scheduler *
     ***********************************************************************/
    vTaskStartScheduler(); /* This function never returns. */

}

/*******************************************************************************
 End of File
 */

