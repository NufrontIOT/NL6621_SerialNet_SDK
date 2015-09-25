/*
******************************************************************************
**
** Project     : 
** Description :    file for application layer task
** Author      :    linhui                             
** Date        : 
**    
** UpdatedBy   : 
** UpdatedDate : 
**
******************************************************************************
*/
/*
******************************************************************************
**                               Include Files
******************************************************************************
*/
#include "includes.h"

#ifdef TEST_APP_SUPPORT
#ifdef LWIP_SUPPORT
#include "lwip/memp.h"
#include "lwIP.h"
#include "lwIP/tcp.h"
#include "lwIP/udp.h"
#include "lwIP/tcpip.h"
#include "netif/etharp.h"
#include "lwIP/dhcp.h"
#include "arch/sys_arch.h"
#include "lwIP/sockets.h"
#include "lwIP/netdb.h"
#include "lwIP/dns.h"

/*
******************************************************************************
**                               MACROS and VARIABLES
******************************************************************************
*/

/*
******************************************************************************
**                               FUNCTION DELEARATIONS
******************************************************************************
*/									  
#ifdef TEST_SERIAL_TO_WIFI

extern SYS_EVT_ID link_status;
extern void ResponseSmartconfig(void);
extern int TestSerialToWifi(void);	
extern int ap_scan_response(void);

#endif	
			 
#ifdef TEST_AIRKISS

extern int TestAirkissFlag;

#endif
/*
******************************************************************************
**                               FUNCTION PROTOTYPES
******************************************************************************
*/

/*
******************************************************************************
**                        VOID AppEvtCallBack(UINT32 event)
**
** Description  : deal with system event, such as linkup/linkdown
** Arguments    : 
                  event: SYS_EVT_ID
                  
** Returns      : нч
** Author       :                                   
** Date         : 
**
******************************************************************************
*/

VOID AppEvtCallBack(SYS_EVT_ID event)
{
	switch (event)
	{
		case SYS_EVT_LINK_UP:
			printf("+EVENT=SYS_EVT_LINK_UP\n\r");

		#ifdef TEST_SERIAL_TO_WIFI
			ResponseSmartconfig();
		#endif

	   	#ifdef TEST_AIRKISS
			TestAirkissFlag = 1;
		#endif
			break;
		
		case SYS_EVT_LINK_DOWN:
			printf("+EVENT=SYS_EVT_LINK_DOWN\n\r");
			break;

		case SYS_EVT_JOIN_FAIL:
			printf("+EVENT=SYS_EVT_JOIN_FAIL\n\r");
			break;

		case SYS_EVT_DHCP_FAIL:
			printf("+EVENT=SYS_EVT_DHCP_FAIL\n\r");
		#ifdef TEST_SERIAL_TO_WIFI
			if(link_status == SYS_EVT_LINK_UP)
			{
				event = SYS_EVT_LINK_UP;
			}
		#endif
			break;

		case SYS_EVT_SCAN_DONE:
			printf("+EVENT=SYS_EVT_SCAN_DONE\n\r");
		#ifdef TEST_SERIAL_TO_WIFI
			if(strlen(&SysParam.WiFiCfg.Ssid) == 0){
				event = SYS_EVT_LINK_DOWN;
			}

			if(link_status == SYS_EVT_LINK_UP)
			{
				event = SYS_EVT_LINK_UP;
			}
			ScanFlag = 1;
		#endif

		#ifdef TEST_AIRKISS
			TestAirkissFlag = 1;
		#endif
			break;

		case SYS_EVT_DIRECT_CFG_DONE:
			printf("+EVENT=SYS_EVT_DIRECT_CFG_DONE\n\r");

		#ifdef TEST_SERIAL_TO_WIFI
			InfWiFiStop();
			InfNetModeSet(PARAM_NET_MODE_STA_BSS);
			InfWiFiStart();
		#endif
			break;

		default:
			break;
	}

#ifdef TEST_SERIAL_TO_WIFI	
	link_status = event;
#endif
}

/*
******************************************************************************
**                        VOID TestAppMain(VOID * pParam)
**
** Description  : application test task
** Arguments    : 
                  pParam
                  
** Returns      : нч
** Author       :                                   
** Date         : 
**
******************************************************************************
*/
VOID TestAppMain(VOID * pParam)
{
#ifdef TEST_SERIAL_TO_WIFI
	TestSerialToWifi();
#endif

    while (1) 
    {                                          /* Task body, always written as an infinite loop.       */
        OSTimeDly(200);
    }
}
#endif // LWIP_SUPPORT
#endif // TEST_APP_SUPPORT

