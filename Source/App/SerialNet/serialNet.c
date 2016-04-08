
#include "common.h"
#include "serialNet.h"
#include "nl6621_uart.h"
#include "ring_buffer.h"

SYS_EVT_ID link_status;

/* User's param */
USER_CFG_PARAM UserParam;
USER_CFG_PARAM FactoryParam;

OS_EVENT *sendSwitchSem;
OS_EVENT * modeSwitchSem;

BOOL_T   isCancelled = OS_FALSE;

UINT8    ScanFlag;
UINT8 smtconfigBegin;
UINT8 	net_sendStart = 0;
//NST_TIMER *Send_Timer = NULL;
extern at_stateType  at_state;
extern UINT8 end_sendFlags;
extern ring_buffer_t uartrxbuf;
extern UINT8 uart_send_flag;

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
	UINT8  sector_num;	//������ַ   GD25q41 0~127
	UINT16 sector_off;	//�������ڵ�ƫ��
	UINT16 secre_main;	//����ʣ��ռ��С   
 	UINT16 i;    
	UINT32 WriteAddr = BurnStartPos;	
    UINT8  *flash_buf = NULL; //flash������
	UINT32 Temporary_var = 0;//��ʱ��������ֹ�Ż�


	sector_num = WriteAddr/FLASH_SECTOR_SIZE;
	sector_off = WriteAddr%FLASH_SECTOR_SIZE;
	secre_main = FLASH_SECTOR_SIZE-sector_off;  
	
	flash_buf = OSMMalloc(FLASH_SECTOR_SIZE); 

	if(flash_buf == NULL)
	{
	   printf("flash_buf malloc fail!\r\n");
	}

	if(DataLen <= secre_main){
		  secre_main = DataLen;//������һ�������ֽ���
	}

	do
	{	
		Temporary_var =  sector_num*FLASH_SECTOR_SIZE;
		QSpiFlashRead(Temporary_var, flash_buf, FLASH_SECTOR_SIZE);//������������������
		for(i=0; i<secre_main; i++){
		    Temporary_var = sector_off+i;
			if(flash_buf[Temporary_var] != 0XFF)  break;//��Ҫ����  	  
		}
		if(i < secre_main){
			Temporary_var = sector_num*FLASH_SECTOR_SIZE;
			QSpiFlashEraseSector(Temporary_var);//�����������
			//OSTimeDly(2);
		    NSTusecDelay(100000); //��ʱ

			for(i=0; i<secre_main; i++){
			    Temporary_var = sector_off+i;
				flash_buf[Temporary_var] = pData[i];	  
			}
		    Temporary_var = sector_num*FLASH_SECTOR_SIZE;
			if(QSpiWriteAny(Temporary_var, flash_buf, FLASH_SECTOR_SIZE) != 0)
			   break;//д����������  
		}
		else{
	    	if(QSpiWriteAny(WriteAddr, pData, secre_main) != 0)//д�Ѿ������˵�,ֱ��д������ʣ������. 
	           break;	
		} 
					   
		if(DataLen == secre_main){
		   break;//д�������
		}
		else{//д��δ����
			sector_num++;//������ַ��1
			sector_off = 0;//ƫ��λ��Ϊ0 	 

		   	pData +=   secre_main;  //ָ��ƫ��
			WriteAddr += secre_main;  //д��ַƫ��	   	   
		   	DataLen -=   secre_main;  //�ֽ����ݼ�
			if(DataLen > FLASH_SECTOR_SIZE) 
			    secre_main = FLASH_SECTOR_SIZE;	//��һ����������д����
			else 
			    secre_main = DataLen;			//��һ����������д����
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
//	UserParam.atMode = DATA_MODE;
//	
//	uart_recvEnd = 0;	
//    uart_rec_len = 0;
//    net_sendStart = 0;
//    net_send_len = 0;
//
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

	if ((UserParam.frameLength < MIN_RECV_BUFFER_SIZE)) {
		UserParam.frameLength = MIN_RECV_BUFFER_SIZE;

	} else if ((UserParam.frameLength > MAX_RECV_BUFFER_SIZE)) {
		UserParam.frameLength = MAX_RECV_BUFFER_SIZE;
	}
	FactoryParam.frameLength = (UserParam.frameLength & 0xffff);
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
	
	
	BSP_GPIOPinMux(UART_RTS);	     
	BSP_GPIOSetDir(UART_RTS, GPIO_DIRECTION_OUTPUT);      
	BSP_GPIOSetValue(UART_RTS, GPIO_HIGH_LEVEL);
	
	BSP_GPIOPinMux(UART_CTS);	     
	BSP_GPIOSetDir(UART_CTS, GPIO_DIRECTION_OUTPUT);      
	BSP_GPIOSetValue(UART_CTS, GPIO_LOW_LEVEL);	 
	
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
 * timer for copying data from uart send buffer to net buffer. 
 **/
void timerFuncProcess(VOID)
{
	OSSemPost(sendSwitchSem);
	if(uart_send_flag != UART_STOP)
	{
		*Tmr0Ctl = (TMR_INT_MASK);
	}	
}

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
void TestSerialToWifi(void * pParam)
{
	UINT8 Err;

	unsigned char prioUser = TCPIP_THREAD_PRIO + 2;
	
	/* create sem viable */
	modeSwitchSem = OSSemCreate(0);
	sendSwitchSem = OSSemCreate(0);

//	NST_InitTimer(&Send_Timer, timerFuncProcess, NULL, NST_TRUE);

	/* create UDP/TCP send and receive process task thread */
	sys_thread_new("SendThread", SendThread, NULL, NST_TEST_APP_TASK_STK_SIZE, prioUser++);
	sys_thread_new("RecvThread", RecvThread, NULL, NST_TEST_APP_TASK_STK_SIZE, prioUser++);

	/* UDP broadcast receive task thread. */
	sys_thread_new("BCTRecvThread", BCTRecvThread, NULL, NST_TEST_APP_TASK_STK_SIZE, prioUser++);

	sys_thread_new("AtThread", AtThread, NULL, NST_TEST_APP_TASK_STK_SIZE, prioUser++);	


	while (1) {    			/* Task body, always written as an infinite loop. */
    	if (UserParam.atMode == AT_MODE) {	/* normal led flash */
			OSTimeDly(100);
			BSP_GPIOSetValue(USER_GPIO_IDX_LED, GPIO_LOW_LEVEL);
			OSTimeDly(100);	
			BSP_GPIOSetValue(USER_GPIO_IDX_LED, GPIO_HIGH_LEVEL);
			
		} else if (UserParam.atMode == DATA_MODE) {						/* serialnet led flash */
			BSP_GPIOSetValue(USER_GPIO_IDX_LED, GPIO_LOW_LEVEL);	
			OSSemPend(modeSwitchSem, 0, &Err);

			/* delay 1s, make sure "+++" string is sand as one frame */
			OSTimeDly(100);
			
			if(end_sendFlags == END_SEND_TRUE){

				printf("+OK:CMDMODE\n");
				end_sendFlags = END_SEND_FALSE;
				net_sendStart = 0;
				UserParam.atMode = AT_MODE;
				ring_buf_clear(&uartrxbuf);
				OSSemPost(sendSwitchSem); 
				at_state = at_statIdle;
				
			}else if(end_sendFlags == END_SEND_FALSE){
				continue;
			} 	
		}
    }
}		/* -----  end of function TestSerialToWifi  ----- */

