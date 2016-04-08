/*
******************************************************************************
**
** Project     : 
** Description :    entry of firware
** Author      :    pchn                             
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
#include "serialNet.h"
#include "nl6621_uart.h"


#pragma arm section zidata="main_only_cpu_access_mem"    /* 仅CPU访问*/
#ifdef LWIP_SUPPORT
static OS_STK   sAppStartTaskStack[NST_APP_START_TASK_STK_SIZE];  
#endif
#pragma arm section

/*
******************************************************************************
**                               FUNCTION DELEARATIONS
******************************************************************************
*/

// Prototypes

extern VOID SystemInit(VOID);
extern SYS_EVT_ID link_status;
extern UINT8 ScanFlag;
extern int TestAirkissFlag;

extern void ResponseSmartconfig(void);
#ifdef LWIP_SUPPORT
extern VOID    TestAppMain(VOID * pParam);
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
                  
** Returns      : 无
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
**                        INT32 main(VOID)
**
** Description  : Application entry point
** Arguments    : 无
** Returns      : 无
** Author       :                                   
** Date         : 
**
******************************************************************************
*/
INT32 main(VOID)
{
    /* system Init */
    SystemInit();

	/* Initialize WIFI base resource */
	InfSysEvtCBSet(AppEvtCallBack);
    InfLoadDefaultParam();

	init_default_data();
	uart_init();

   /* create application tasks here */
#ifdef TEST_APP_SUPPORT
#ifdef LWIP_SUPPORT
    OSTaskCreate(TestSerialToWifi, NULL, &sAppStartTaskStack[NST_APP_START_TASK_STK_SIZE -1],  NST_APP_TASK_START_PRIO); 
#endif // LWIP_SUPPORT
#endif //TEST_APP_SUPPORT

    /* Start Os */
    OSStart();
    
    return 1;
}


/*
******************************************************************************
**                            End Of File                                   **
******************************************************************************
*/


