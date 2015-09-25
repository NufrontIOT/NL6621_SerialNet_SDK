/*
 * =====================================================================================
 *
 *       Filename:  serialNet.c
 *
 *    Description:  The entry of serialnet modle. init serialnet resource and create
 *              process task thread.
 *
 *        Version:  0.01.1
 *        Created:  11/11/2014 12:57:35 AM
 *       Revision:  none
 *
 *         Author:  Lin Hui (Link), hui.lin@nufront.com
 *   Organization:  Guangdong Nufront CSC Co., Ltd
 *
 *--------------------------------------------------------------------------------------          
 * ChangLog:
 *  version    Author      Date        Purpose
 *  0.01.01     Lin Hui    11/11/2014   Create and initialize it.   
 *  0.01.02     Lin Hui    10/12/2014   Fix the bug in internal release SerialNet 0.01.02 
 *     
 * =====================================================================================
 */

#include "serialNet.h"
#include "common.h"

UINT8 	uart_recvEnd = 0;
UINT32	uart_rec_len = 0;
char 	uart_rec_data[MAX_RECV_BUFFER_SIZE + 1];
UINT8 	net_sendStart = 0;
UINT32	net_send_len = 0;
char 	net_send_data[MAX_RECV_BUFFER_SIZE + 1];
UINT8 	uart_sendStart = 0;				
UINT8	uart_hasData = 0;						/*0: 无数据发送 1:接收到数据 */
char 	uart_send_data[MAX_RECV_BUFFER_SIZE + 1];
UINT32	uart_send_len = 0;
UINT8	end_sendFlags = 0;
NST_TIMER *pTimer = NULL;
BOOL_T   isCancelled = OS_FALSE;
UINT8    ScanFlag;



SYS_EVT_ID link_status;
UINT8 smtconfigBegin;

UINT32 g_RecvBufSize = DEF_RECV_BUFFER_SIZE;

/* User's param */
USER_CFG_PARAM UserParam;
USER_CFG_PARAM FactoryParam;

OS_EVENT *modeSwitchSem;  	/* command and data mode switch semaphore. */
OS_EVENT *uartMessgSem;  	/* Uart error message print sem */


/***************************************************************************/
/************** factory and user's parameters save interface ***************/
/***************************************************************************/

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ReadFlash
 *  Description:  Read data from nor flash. 
 * =====================================================================================
 */
static VOID ReadFlash(UINT8* pBuf, UINT32 DataLen, UINT32 ReadStartPos)
{
    QSpiFlashRead(ReadStartPos, pBuf, DataLen);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  writeFlash
 *  Description:  write data to nor flash 
 * =====================================================================================
 */
static VOID WriteFlash(UINT8* pData, UINT32 DataLen, UINT32 BurnStartPos)
{
	UINT8  sector_num;	//扇区地址   GD25q41 0~127
	UINT16 sector_off;	//在扇区内的偏移
	UINT16 secre_main;	//扇区剩余空间大小   
 	UINT16 i;    
	UINT32 WriteAddr = BurnStartPos;	
    UINT8  *flash_buf = NULL; //flash缓冲区
	UINT32 Temporary_var = 0;//临时变量，防止优化


	sector_num = WriteAddr/FLASH_SECTOR_SIZE;
	sector_off = WriteAddr%FLASH_SECTOR_SIZE;
	secre_main = FLASH_SECTOR_SIZE-sector_off;  
	
	flash_buf = OSMMalloc(FLASH_SECTOR_SIZE); 

	if(flash_buf == NULL)
	{
	   printf("flash_buf malloc fail!\r\n");
	}

	if(DataLen <= secre_main){
		  secre_main = DataLen;//不大于一个扇区字节数
	}

	do
	{	
		Temporary_var =  sector_num*FLASH_SECTOR_SIZE;
		QSpiFlashRead(Temporary_var, flash_buf, FLASH_SECTOR_SIZE);//读出整个扇区的内容
		for(i=0; i<secre_main; i++){
		    Temporary_var = sector_off+i;
			if(flash_buf[Temporary_var] != 0XFF)  break;//需要擦除  	  
		}
		if(i < secre_main){
			Temporary_var = sector_num*FLASH_SECTOR_SIZE;
			QSpiFlashEraseSector(Temporary_var);//擦除这个扇区
			//OSTimeDly(2);
		    NSTusecDelay(100000); //延时

			for(i=0; i<secre_main; i++){
			    Temporary_var = sector_off+i;
				flash_buf[Temporary_var] = pData[i];	  
			}
		    Temporary_var = sector_num*FLASH_SECTOR_SIZE;
			if(QSpiWriteAny(Temporary_var, flash_buf, FLASH_SECTOR_SIZE) != 0)
			   break;//写入整个扇区  
		}
		else{
	    	if(QSpiWriteAny(WriteAddr, pData, secre_main) != 0)//写已经擦除了的,直接写入扇区剩余区间. 
	           break;	
		} 
					   
		if(DataLen == secre_main){
		   break;//写入结束了
		}
		else{//写入未结束
			sector_num++;//扇区地址增1
			sector_off = 0;//偏移位置为0 	 

		   	pData +=   secre_main;  //指针偏移
			WriteAddr += secre_main;  //写地址偏移	   	   
		   	DataLen -=   secre_main;  //字节数递减
			if(DataLen > FLASH_SECTOR_SIZE) 
			    secre_main = FLASH_SECTOR_SIZE;	//下一个扇区还是写不完
			else 
			    secre_main = DataLen;			//下一个扇区可以写完了
		}	 
	}while(1); 
	
	OSMFree(flash_buf); 	
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GetFactoryParam
 *  Description:  Get factory parameters from nor flash. 
 * =====================================================================================
 */
VOID GetFactoryParam(VOID)
{
	ReadFlash((UINT8*)&FactoryParam, sizeof(USER_CFG_PARAM), FACTORY_PARAM_ADDR);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  saveFactoryParam
 *  Description:  Save factory parameters to nor flash, 200k bytes offset.
 * =====================================================================================
 */
VOID SaveFactoryParam(VOID)
{
	WriteFlash((UINT8*)&FactoryParam, sizeof(USER_CFG_PARAM), FACTORY_PARAM_ADDR);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GetUserParam
 *  Description:  Get user's parameters from nor flash, 210k bytes offset. 
 * =====================================================================================
 */
VOID GetUserParam(VOID)
{
	ReadFlash((UINT8*)&UserParam, sizeof(USER_CFG_PARAM), USER_PARAM_ADDR);

	memcpy(&SysParam, &UserParam.cfg, sizeof(CFG_PARAM));
}		/* -----  end of function GetUserParam  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  SaveUserParam
 *  Description:  Save user's parameters to nor flash, 210k bytes offset. 
 * =====================================================================================
 */
VOID SaveUserParam(VOID)
{
	WriteFlash((UINT8*)&UserParam, sizeof(USER_CFG_PARAM), USER_PARAM_ADDR);
}		/* -----  end of function SaveUserParam  ----- */




/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  InitFactoryParam
 *  Description:  Initialize factory pamameters, which will save in factory section on
 *              nor flash. This function will set the default parameters when no user's
 *              load parameters.
 *         Note:
 * =====================================================================================
 */
VOID InitFactoryParam(VOID)
{
	UINT32 remoteCommIp;

	/* clear struct */
	memset((void*)&FactoryParam, 0, sizeof(USER_CFG_PARAM));

	/* set user parameters valid flag */	
	FactoryParam.isValid = USER_PARAM_FLAG_INVALID; 

	/* set default sleep mode about after launch: normal mode */
	FactoryParam.SleepMode = NORMAL_MODE;

	/* set default mode about after launch: AT mode */
	FactoryParam.atMode = AT_MODE;

	/* set UART default baudrate: 115200 */
	FactoryParam.Baudrate = DEFAULT_UART_BAUDRATE;

	/* set default as server/client */
	FactoryParam.socketType = SOCK_SERVER;

	/* set default socket connect type, tcp/udp */
	FactoryParam.socketProtocol = TCP;
	
	/* remote socket Port */
	FactoryParam.socketPort = DEFAULT_REMOTE_COMM_PORT;

	/* set default remote ip address */
	inet_aton(DEFAULT_REMOTE_COMM_IP, &remoteCommIp);
	FactoryParam.remoteCommIp = remoteCommIp;

	/* set default receive buffer size and send gap time */
	FactoryParam.frameLength = DEF_RECV_BUFFER_SIZE;
	FactoryParam.frameGap = DEFAULT_SEND_DATA_GAP; 

	/* load default sysParam */
	InfLoadDefaultParam();
	
	/* Save the default CFG_PARAM data */
	memcpy(&FactoryParam.cfg, &SysParam, sizeof(SysParam));
}		/* -----  end of function InitFactoryParam  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  Init_SeriaNet
 *  Description:  Read flash, default boot mode. 
 *         Note:
 * =====================================================================================
 */
 void Init_SeriaNet(void)
{
	//UserParam.atMode = DATA_MODE;
	
	uart_recvEnd = 0;	
    uart_rec_len = 0;
    net_sendStart = 0;
    net_send_len = 0;

    /* start to create TCP/UDP connect socket */
    memset(&WifiConnStatus, 0, sizeof(WifiConnStatus));
    WifiConnStatus.connStatus = CONNECT_STATUS_FAIL;
    WifiConnStatus.socketProtocol = UserParam.socketProtocol;
    WifiConnStatus.socketType = UserParam.socketType;
    WifiConnStatus.socketPort = UserParam.socketPort;
    WifiConnStatus.remoteIp = UserParam.remoteCommIp;
    WifiConnStatus.sockfd = -1;

	InfWiFiStart();
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LoadUserParam
 *  Description:  Load user's parameters, which will initializate user's parameters, set
 *              to normal sleep mode, AT mode, judge uart fram size and time gap.
 *         Note:
 * =====================================================================================
 */
VOID LoadUserParam(VOID)
{
	UserParam.SleepMode = NORMAL_MODE;	/* set to normal mode */
	
	/* set at mode */
	UserParam.atMode = AT_MODE;

	/* get the system param from user mode */
	memcpy(&SysParam, &UserParam.cfg, sizeof(CFG_PARAM));

	g_RecvBufSize = UserParam.frameLength;
	if ((UserParam.frameLength < MIN_RECV_BUFFER_SIZE)) {
		g_RecvBufSize = MIN_RECV_BUFFER_SIZE;

	} else if ((UserParam.frameLength > MAX_RECV_BUFFER_SIZE)) {
		g_RecvBufSize = MAX_RECV_BUFFER_SIZE;
	}
	FactoryParam.frameLength = (g_RecvBufSize & 0xffff);
}		/* -----  end of function LoadUserParam  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  UserGpioInit
 *  Description:  Initialize factory gpio, led indicator gpio. 
 *         Note:
 * =====================================================================================
 */
VOID UserGpioInit(VOID)
{
	/* set indicator LED to low level */     
	BSP_GPIOPinMux(USER_GPIO_IDX_LED);	     
	BSP_GPIOSetDir(USER_GPIO_IDX_LED, GPIO_DIRECTION_OUTPUT);      
	BSP_GPIOSetValue(USER_GPIO_IDX_LED, GPIO_LOW_LEVEL);	 
	
	/* factory gpio is valied when set to low level */
	BSP_GPIOPinMux(USER_GPIO_FACTORY);  
	BSP_GPIOSetDir(USER_GPIO_FACTORY, GPIO_DIRECTION_INPUT); 
}		/* -----  end of function UserGpioInit  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GlobalInit
 *  Description:  Init device global vialable. 
 * =====================================================================================
 */
VOID GlobalInit(VOID)
{	
	UserGpioInit();

	link_status = SYS_EVT_LINK_DOWN;
				   	
	uart_recvEnd = 0;
	uart_rec_len = 0;
	net_sendStart = 0;
	net_send_len = 0;

	memset(uart_rec_data, '\0', MAX_RECV_BUFFER_SIZE);
	memset(net_send_data, '\0', MAX_RECV_BUFFER_SIZE);
}		/* -----  end of function GlobalInit  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  init_default_data
 *  Description:  Init device to default value, if the factory gpio was set to low, it 
 *              will reset user's parameters to factory parameters. Or it will reload 
 *              to user's parameters from nor flash, if the user's parameters is valid,
 *              set it to current enviroment, otherwise, reset to factory mode.
 *         Note:
 * =====================================================================================
 */
int init_default_data(void)
{
	UINT8 gpio_val;

	GlobalInit();
	
	/* if the factory button was pressed(gpio is low level), then set to factory mode,
	 * else load user parameters.
	 **/
	gpio_val = BSP_GPIOGetValue(USER_GPIO_FACTORY);
	if (gpio_val == 0) {
		NSTusecDelay(50000);		/* delay 50ms filter button shake */
		gpio_val = BSP_GPIOGetValue(USER_GPIO_FACTORY);
		if (gpio_val == 0) {	
			InitFactoryParam();
			SaveFactoryParam();
			GetFactoryParam();
			memcpy((void *)&UserParam, (void *)&FactoryParam, sizeof(USER_CFG_PARAM));
			SaveUserParam();

			BSP_UartOpen(FactoryParam.Baudrate);
			NSTusecDelay(20000);
			printf("Load factory parameters(Hardware).\n"); 

			return 0;
		} 
	}	
		
	/* 
	 * If user's validflag parameters is set to 0x01020304, then
	 * load user's parameters, otherwise, load the factory parameters.
	 */
	GetUserParam(); 
	if (UserParam.isValid == USER_PARAM_FLAG_VALID) {
		LoadUserParam();

		BSP_UartOpen(UserParam.Baudrate);
		NSTusecDelay(20000);
		printf("Load user's parameters.\r\n");
		Init_SeriaNet();

	} else {
		InitFactoryParam();
		SaveFactoryParam();
		GetFactoryParam();
		memcpy((void *)&UserParam, (void *)&FactoryParam, sizeof(USER_CFG_PARAM));
		SaveUserParam();

		BSP_UartOpen(FactoryParam.Baudrate);
		NSTusecDelay(20000);
		printf("Load factory parameters(Software).\r\n");
	}

	return 0; 
}		/* -----  end of function init_default_data  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  TestSerialToWifi
 *  Description:  The entry of serialnet module, it will create the uart and wifi net task.
 *              after that it will be polled to reflash led in AT mode, or light on 
 *              indicator LED then wait to switch to AT mode when receive "+++" from uart
 *              in data mode.
 *         Note:
 * =====================================================================================
 */
int TestSerialToWifi(void)
{
	UINT8 Err;
	UINT8 prioUser = TCPIP_THREAD_PRIO + 2;		  

	/* create sem viable */
	modeSwitchSem = OSSemCreate(0);

	sys_thread_new("UartRecvThread", UartRecvThread, NULL, NST_TEST_APP_TASK_STK_SIZE, prioUser++);//3
	sys_thread_new("UartSendThread", UartSendThread, NULL, NST_TEST_APP_TASK_STK_SIZE, prioUser++);//4	
	sys_thread_new("AtThread", AtThread, NULL, NST_TEST_APP_TASK_STK_SIZE, prioUser++);//5

	/* create UDP/TCP send and receive process task thread */
	sys_thread_new("SendThread", SendThread, NULL, NST_TEST_APP_TASK_STK_SIZE, prioUser++);//6
	sys_thread_new("RecvThread", RecvThread, NULL, NST_TEST_APP_TASK_STK_SIZE, prioUser++);//7

	/* UDP broadcast receive task thread. */
	sys_thread_new("BCTRecvThread", BCTRecvThread, NULL, NST_TEST_APP_TASK_STK_SIZE, prioUser++);//8

	while (1) {    			/* Task body, always written as an infinite loop. */
    	if (UserParam.atMode == 0) {	/* normal led flash */
			OSTimeDly(100);
			BSP_GPIOSetValue(USER_GPIO_IDX_LED, GPIO_LOW_LEVEL);
			OSTimeDly(100);	
			BSP_GPIOSetValue(USER_GPIO_IDX_LED, GPIO_HIGH_LEVEL);
			
		} else if (UserParam.atMode == 1) {						/* serialnet led flash */
			BSP_GPIOSetValue(USER_GPIO_IDX_LED, GPIO_LOW_LEVEL);	
			OSSemPend(modeSwitchSem, 0, &Err);

			/* delay 100ms, make sure "+++" string is sand as one frame */
			OSTimeDly(10); 	
	
			if ((uart_rec_len <= 5)) {	
				uart_rec_data[3] = '\0';
				if (strcmp(uart_rec_data, "+++") == 0) {
					/* clear data mode */								
					uart_recvEnd = 0;	
					uart_rec_len = 0;	
					net_sendStart = 0;
					net_send_len = 0;
					UserParam.atMode = AT_MODE;
					
					printf("+OK:CMDMODE\n");
				}
			}
		}
    }
}		/* -----  end of function TestSerialToWifi  ----- */




/* ************************************************************************ */
/* ********************* External called interface  *********************** */
/* ************************************************************************ */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ResponseSmartconfig
 *  Description:  When start smart config done, this interface will be called. It will
 *              set and save user's parameters.
 *         Note:
 * =====================================================================================
 */
void ResponseSmartconfig(void)
{
	if (smtconfigBegin == 1) {
		smtconfigBegin = 0;
		/* after smartconfig success, save user's parameter */
		memcpy(&UserParam.cfg, &SysParam, sizeof(CFG_PARAM));
		//UserParam.isValid = USER_PARAM_FLAG_VALID;	
		//SaveUserParam();
		
		if (LWIP_DHCP) {
			printf("+OK:IP=%s\n\r", inet_ntoa(netif.ip_addr.addr));
			
			/* UDP broadcast one time */
			RespondUdpBroadcast();

		} else {
			printf("+ERROR=%d\n\r", INVALID_LINKUP_FAILED);
		}
	}
}		/* -----  end of function ResponseSmartconfig  ----- */





