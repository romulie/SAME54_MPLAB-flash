/*
 * ftp_startup.c
 *
 * Created: may 2024
 *  Author: PI
 */ 

#include "ftp_startup.h"

#include "core/net.h"

#include "same54_eth_driver.h"
#include "ksz8091_driver.h"

#include "dhcp/dhcp_client.h"
#include "ipv6/slaac.h"
#include "ftp/ftp_client.h"
#include "ftp/ftp_server.h"

#include "debug.h"
#include "definitions.h"                // SYS function prototypes
#include "configuration.h"

//Ethernet interface configuration
// TODO: create ftp_startup_cfg.h file and move the following there
#define APP_IF_NAME "eth0"
#define APP_HOST_NAME "ftp-server-demo"
#define APP_MAC_ADDR "00-AB-CD-EF-54-20"

#define APP_USE_DHCP_CLIENT ENABLED
#define APP_IPV4_HOST_ADDR "192.168.0.20"
#define APP_IPV4_SUBNET_MASK "255.255.255.0"
#define APP_IPV4_DEFAULT_GATEWAY "192.168.0.254"
#define APP_IPV4_PRIMARY_DNS "8.8.8.8"
#define APP_IPV4_SECONDARY_DNS "8.8.4.4"

#define APP_USE_SLAAC ENABLED
#define APP_IPV6_LINK_LOCAL_ADDR "fe80::5420"
#define APP_IPV6_PREFIX "2001:db8::"
#define APP_IPV6_PREFIX_LENGTH 64
#define APP_IPV6_GLOBAL_ADDR "2001:db8::5420"
#define APP_IPV6_ROUTER "fe80::1"
#define APP_IPV6_PRIMARY_DNS "2001:4860:4860::8888"
#define APP_IPV6_SECONDARY_DNS "2001:4860:4860::8844"

//Application configuration: this pertains the remote FTP-server for demo purposes
#define APP_FTP_SERVER_NAME "test.rebex.net"
#define APP_FTP_SERVER_PORT 21
#define APP_FTP_LOGIN "demo"
#define APP_FTP_PASSWORD "password"
#define APP_FTP_FILENAME "readme.txt"

//Application configuration: this pertains the local FTP-server
#define APP_FTP_LOCAL_SERVER_MAX_CONNECTIONS         2

//Global variables
DhcpClientSettings dhcpClientSettings;
DhcpClientContext dhcpClientContext;
SlaacSettings slaacSettings;
SlaacContext slaacContext;

FtpClientContext ftpClientContext;

// Local FTP-server-related global variables
FtpServerSettings ftpServerSettings;
FtpServerContext ftpServerContext;
FtpClientConnection ftpConnections[APP_FTP_LOCAL_SERVER_MAX_CONNECTIONS];

//TODO: Forward declaration of ftp-server call-back functions
error_t ftpConnectCallback(FtpClientConnection *connection, const IpAddr *clientIpAddr, uint16_t clientPort)
{
    TRACE_INFO("FTP connection callback. Client port = %d\r\n", clientPort);
    return 0;
}

void ftpDisconnectCallback(FtpClientConnection *connection, const IpAddr *clientIpAddr, uint16_t clientPort)
{
    TRACE_INFO("FTP disconnection callback. Client port = %d\r\n", clientPort);
    return;
}

uint_t ftpCheckUserCallback(FtpClientConnection *connection, const char_t *user)
{
    TRACE_INFO("***********FTP_CALLBACK: FTP check user callback. user = %s\r\n", user);
    return 1;
}

uint_t ftpCheckPasswordCallback(FtpClientConnection *connection, const char_t *user, const char_t *password)
{
    TRACE_INFO("***********FTP_CALLBACK: FTP check password callback. user = %s, password = %s\r\n", user, password);
    return 1;
}
uint_t ftpGetFilePermCallback(FtpClientConnection *connection, const char_t *user, const char_t *path)
{
    uint_t permission = FTP_FILE_PERM_LIST | FTP_FILE_PERM_READ | FTP_FILE_PERM_WRITE;
    TRACE_INFO("***********FTP_CALLBACK: FTP file permission callback. user = %s, path = %s\r\n", user, path);
    return permission;
}

error_t ftpUnknownCommandCallback(FtpClientConnection *connection, const char_t *command, const char_t *param)
{
    TRACE_INFO("***********FTP_CALLBACK: FTP unknown command callback. command = %s, param = %s\r\n", command, param);
    return 0;
}
//=========================================================
/**
 * @brief FTP client test routine
 * @return Error code
 **/

error_t ftpClientTest(void)
{
   error_t error;
   size_t n;
   IpAddr ipAddr;
   char_t buffer[128];

   //Initialize FTP client context
   ftpClientInit(&ftpClientContext);

   //Start of exception handling block
   do
   {
      //Debug message
      TRACE_INFO("\r\n\r\nResolving server name...\r\n");

      //Resolve FTP server name
      error = getHostByName(NULL, APP_FTP_SERVER_NAME, &ipAddr, 0);
      //Any error to report?
      if(error)
      {
         //Debug message
         TRACE_INFO("Failed to resolve server name!\r\n");
         break;
      }

      //Set timeout value for blocking operations
      error = ftpClientSetTimeout(&ftpClientContext, 20000);
      //Any error to report?
      if(error)
         break;

      //Debug message
      TRACE_INFO("Connecting to FTP server %s...\r\n",
         ipAddrToString(&ipAddr, NULL));

      //Connect to the FTP server
      error = ftpClientConnect(&ftpClientContext, &ipAddr, APP_FTP_SERVER_PORT,
         FTP_MODE_PLAINTEXT | FTP_MODE_PASSIVE);
      //Any error to report?
      if(error)
      {
         //Debug message
         TRACE_INFO("Failed to connect to FTP server!\r\n");
         break;
      }

      //Login to the FTP server using the provided username and password
      error = ftpClientLogin(&ftpClientContext, APP_FTP_LOGIN, APP_FTP_PASSWORD);
      //Any error to report?
      if(error)
         break;

      //Open the specified file for reading
      error = ftpClientOpenFile(&ftpClientContext, APP_FTP_FILENAME,
         FTP_FILE_MODE_READ | FTP_FILE_MODE_BINARY);
      //Any error to report?
      if(error)
         break;

      //Read the contents of the file
      while(!error)
      {
         //Read data
         error = ftpClientReadFile(&ftpClientContext, buffer, sizeof(buffer) - 1, &n, 0);

         //Check status code
         if(!error)
         {
            //Properly terminate the string with a NULL character
            buffer[n] = '\0';
            //Dump the contents of the file
            TRACE_INFO("%s", buffer);
         }
      }

      //Terminate the string with a line feed
      TRACE_INFO("\r\n");

      //Any error to report?
      if(error != ERROR_END_OF_STREAM)
         break;

      //Close file
      error = ftpClientCloseFile(&ftpClientContext);
      //Any error to report?
      if(error)
         break;

      //Gracefully disconnect from the FTP server
      ftpClientDisconnect(&ftpClientContext);

      //Debug message
      TRACE_INFO("Connection closed\r\n");

      //End of exception handling block
   } while(0);

   //Release FTP client context
   ftpClientDeinit(&ftpClientContext);

   //Return status code
   return error;
}


/**
 * @brief User task
 * @param[in] param Unused parameter
 **/

void userTask(void *param)
{
   //Endless loop
   while(1)
   {
      //SW0 button pressed?
      if(!(PORT_REGS->GROUP[1].PORT_IN & (1U << 31)))
      {
         //FTP client test routine
         ftpClientTest();

         //Wait for the SW0 button to be released
         while(!(PORT_REGS->GROUP[1].PORT_IN & (1U << 31)));
      }

      //Loop delay
      osDelayTask(100);
   }
}

//=========================================================
uint8_t Ftp_Startup(void)
{
    error_t error;
    OsTaskId taskId;
    OsTaskParameters taskParams;
    NetInterface *interface;
    MacAddr macAddr;
#if (APP_USE_DHCP_CLIENT == DISABLED)
    Ipv4Addr ipv4Addr;
#endif
#if (APP_USE_SLAAC == DISABLED)
    Ipv6Addr ipv6Addr;
#endif
    
    //Initiate FileSystem
    error = fsInit();
    if(error)
    {
        //Debug message
        TRACE_ERROR("Failed to initialize filesystem!\r\n");
    }
    
    //TCP/IP stack initialization
    error = netInit();
    //Any error to report?
    if(error)
    {
        //Debug message
        TRACE_ERROR("Failed to initialize TCP/IP stack!\r\n");
    }

    //Configure the first Ethernet interface
    interface = &netInterface[0];

    //Set interface name
    netSetInterfaceName(interface, APP_IF_NAME);
    //Set host name
    netSetHostname(interface, APP_HOST_NAME);
    //Set host MAC address
    macStringToAddr(APP_MAC_ADDR, &macAddr);
    netSetMacAddr(interface, &macAddr);
    //Select the relevant network adapter
    netSetDriver(interface, &same54EthDriver);
    netSetPhyDriver(interface, &ksz8091PhyDriver);

    //Initialize network interface
    error = netConfigInterface(interface);
    //Any error to report?
    if(error)
    {
        //Debug message
        TRACE_ERROR("Failed to configure interface %s!\r\n", interface->name);
    }

#if (IPV4_SUPPORT == ENABLED)
  #if (APP_USE_DHCP_CLIENT == ENABLED)
    //Get default settings
    dhcpClientGetDefaultSettings(&dhcpClientSettings);
    //Set the network interface to be configured by DHCP
    dhcpClientSettings.interface = interface;
    //Disable rapid commit option
    dhcpClientSettings.rapidCommit = FALSE;

    //DHCP client initialization
    error = dhcpClientInit(&dhcpClientContext, &dhcpClientSettings);
    //Failed to initialize DHCP client?
    if(error)
    {
        //Debug message
        TRACE_ERROR("Failed to initialize DHCP client!\r\n");
    }

    //Start DHCP client
    error = dhcpClientStart(&dhcpClientContext);
    //Failed to start DHCP client?
    if(error)
    {
        //Debug message
        TRACE_ERROR("Failed to start DHCP client!\r\n");
    }
  #else
    //Set IPv4 host address
    ipv4StringToAddr(APP_IPV4_HOST_ADDR, &ipv4Addr);
    ipv4SetHostAddr(interface, ipv4Addr);

    //Set subnet mask
    ipv4StringToAddr(APP_IPV4_SUBNET_MASK, &ipv4Addr);
    ipv4SetSubnetMask(interface, ipv4Addr);

    //Set default gateway
    ipv4StringToAddr(APP_IPV4_DEFAULT_GATEWAY, &ipv4Addr);
    ipv4SetDefaultGateway(interface, ipv4Addr);

    //Set primary and secondary DNS servers
    ipv4StringToAddr(APP_IPV4_PRIMARY_DNS, &ipv4Addr);
    ipv4SetDnsServer(interface, 0, ipv4Addr);
    ipv4StringToAddr(APP_IPV4_SECONDARY_DNS, &ipv4Addr);
    ipv4SetDnsServer(interface, 1, ipv4Addr);
  #endif
#endif

#if (IPV6_SUPPORT == ENABLED)
  #if (APP_USE_SLAAC == ENABLED)
    //Get default settings
    slaacGetDefaultSettings(&slaacSettings);
    //Set the network interface to be configured
    slaacSettings.interface = interface;

    //SLAAC initialization
    error = slaacInit(&slaacContext, &slaacSettings);
    //Failed to initialize SLAAC?
    if(error)
    {
        //Debug message
        TRACE_ERROR("Failed to initialize SLAAC!\r\n");
    }

    //Start IPv6 address autoconfiguration process
    error = slaacStart(&slaacContext);
    //Failed to start SLAAC process?
    if(error)
    {
        //Debug message
        TRACE_ERROR("Failed to start SLAAC!\r\n");
    }
  #else
    //Set link-local address
    ipv6StringToAddr(APP_IPV6_LINK_LOCAL_ADDR, &ipv6Addr);
    ipv6SetLinkLocalAddr(interface, &ipv6Addr);

    //Set IPv6 prefix
    ipv6StringToAddr(APP_IPV6_PREFIX, &ipv6Addr);
    ipv6SetPrefix(interface, 0, &ipv6Addr, APP_IPV6_PREFIX_LENGTH);

    //Set global address
    ipv6StringToAddr(APP_IPV6_GLOBAL_ADDR, &ipv6Addr);
    ipv6SetGlobalAddr(interface, 0, &ipv6Addr);

    //Set default router
    ipv6StringToAddr(APP_IPV6_ROUTER, &ipv6Addr);
    ipv6SetDefaultRouter(interface, 0, &ipv6Addr);

    //Set primary and secondary DNS servers
    ipv6StringToAddr(APP_IPV6_PRIMARY_DNS, &ipv6Addr);
    ipv6SetDnsServer(interface, 0, &ipv6Addr);
    ipv6StringToAddr(APP_IPV6_SECONDARY_DNS, &ipv6Addr);
    ipv6SetDnsServer(interface, 1, &ipv6Addr);
  #endif
#endif

    //Set task parameters
    taskParams = OS_TASK_DEFAULT_PARAMS;
    taskParams.stackSize = 500;
    taskParams.priority = OS_TASK_PRIORITY_NORMAL;

    //Create user task (visit remote test FTP server upon SW0 button press)
    taskId = osCreateTask("User", userTask, NULL, &taskParams);
    //Failed to create the task?
    if(taskId == OS_INVALID_TASK_ID)
    {
        //Debug message
        TRACE_ERROR("Failed to create task!\r\n");
    }

//==========================================================================
    //TRACE_INFO("About to SET ftp server settings........\r\n");
    //Get default settings
    ftpServerGetDefaultSettings(&ftpServerSettings);
    //Bind FTP server to the desired interface
    ftpServerSettings.interface = &netInterface[0];
    //FTP command port 21
    ftpServerSettings.port = FTP_PORT;
    //FTP data port 20
    ftpServerSettings.dataPort = FTP_DATA_PORT;
    //Passive port range (lower value)
    ftpServerSettings.passivePortMin = 1024;
    //Passive port range (upper value)
    ftpServerSettings.passivePortMax = 5000;
    //Public IPv4 address to be used in PASV replies
    ftpServerSettings.publicIpv4Addr = ((192 << 24) + (168 << 16) + (100 << 8) + 19);   //???? 192.168.100.19
    //Security modes
    ftpServerSettings.mode = FTP_SERVER_MODE_PLAINTEXT;                                 //???? =1
    //Maximum number of client connections
    ftpServerSettings.maxConnections = APP_FTP_LOCAL_SERVER_MAX_CONNECTIONS;
    ftpServerSettings.connections = ftpConnections;
    //Specify the server's root directory
    strcpy(ftpServerSettings.rootDir, "/");                     //????
    //Callback functions
    ftpServerSettings.connectCallback           = ftpConnectCallback;
    ftpServerSettings.disconnectCallback        = ftpDisconnectCallback;
    ftpServerSettings.checkUserCallback         = ftpCheckUserCallback;           ///<User verification callback function
    ftpServerSettings.checkPasswordCallback     = ftpCheckPasswordCallback;       ///<Password verification callback function
    ftpServerSettings.getFilePermCallback       = ftpGetFilePermCallback;         ///<Callback used to retrieve file permissions
    ftpServerSettings.unknownCommandCallback    = ftpUnknownCommandCallback;      ///<Unknown command callback function

    //TRACE_INFO("--------------------------About to INIT ftp server!\r\n");

    //FTP server initialization
    error = ftpServerInit(&ftpServerContext, &ftpServerSettings);
    //Failed to initialize FTP server?
    if(error)
    {
        //Debug message
        TRACE_ERROR("Failed to initialize FTP server!\r\n");
    }
    //TRACE_INFO("About to START ftp server.......\r\n");
    //Start FTP server
    error = ftpServerStart(&ftpServerContext);
    //Failed to start FTP server?
    if(error)
    {
        //Debug message
        TRACE_ERROR("-------------------FTP server start FAILED!\r\n");
    }

    return error;
}