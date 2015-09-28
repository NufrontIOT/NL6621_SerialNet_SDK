#ifndef __COMMON_H__
#define __COMMON_H__

#include "includes.h"
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

#include "lwIP/snmp.h"
#include "lwIP/stats.h"
#include "lwip/inet_chksum.h"  /* add for inet_chksum interface */

#include "serialNet.h"
#include "socketCom.h"
#include "at_cmd.h"
#include "ping.h"

#define USER_PARAM_FLAG_INVALID    	(0x0)	   			/* User parameters flag Macro */
#define USER_PARAM_FLAG_VALID    	(0x01020304)	   	/* User parameters flag Macro */

#define USER_GPIO_MAX				(3)

#define USER_GPIO_IDX_LED  			(9)    				/* indicater LED gpio */ 
#define USER_GPIO_FACTORY 			(10)				/* factory mode gpio */ 

#define FACTORY_PARAM_ADDR      	(0x64000) 			/* pactory's parameters address with 400k bytes offset */
#define USER_PARAM_ADDR 			(0x65000)			/* user's parameters address with 420k bytes offset */

#define DEFAULT_REMOTE_COMM_PORT  	(8101)
#define DEFAULT_REMOTE_COMM_IP 		"192.168.0.100"

#define MAX_RECV_BUFFER_SIZE 		(1400)
#define MIN_RECV_BUFFER_SIZE 		(32)
#define DEF_RECV_BUFFER_SIZE 		(1024)

#define DEFAULT_RECV_BUFFER_SIZE    (256)

#define DEFAULT_SEND_DATA_GAP 		(100)		/* 100ms */

//#pragma pack(1)

typedef enum{
	SUCCESS   			   = 0,
	INVALID_COMMAND 	   = -1,
	INVALID_PARAMETER	   = -2,

	INVALID_SCAN_AP_RUNNING = -8,
	INVALID_SCAN_AP_FAIL   = -9,
	INVALID_WMODE	       = -10,
	INVALID_NMODE		   = -11,
	INVALID_NTYPE          = -12,	 
	INVALID_LOCIP_SUPPORT  = -13,
	INVALID_SEND_NOT_OVER  = -14,
	INVALID_LINKUP_FAILED  = -15,
	INVALID_PING_RUNNING   = -16,
	INVALID_SMTCONF_RUNNING = -17,
	INVALID_DATA_LINKDOWN  = -18,
	INVALID_WPHYMODE		= -19,

	INVALID_SOCK_PROTOCAL  = -20,
	INVALID_SOCK_TYPE      = -21,
	INVALID_TCP_LISTEN     = -22,
	INVALID_TCP_CONNECT    = -23,
	INVALID_TCP_SETOPT     = -24,

	INVALID_BIND	       = -30,
	INVALID_UDP_SOCKET	   = -31,
	INVALID_TCP_SOCKET	   = -32,
	INVALID_SEND	       = -33,
	INVALID_RECV	       = -34,

	INVALID_BCTTX_RUNNING_OR_LINKED  = -40,
	INVALID_BCTTX_SOCK     = -41,
	INVALID_BCTTX_SETOPT   = -42,
	INVALID_BCTTX_SEND     = -43,

	INVALID_BCTRX_RUNNING_OR_LINKED  = -45,
	INVALID_BCTRX_SOCK     = -46,
	INVALID_BCTRX_BIND	   = -47,
	INVALID_BCTRX_RECV     = -48,
	INVALID_BCTRX_SELECT   = -49,

}ERROR_TYPE;

typedef enum{
	UDP = 0,
	TCP = 1,
}SOCKETPROTOCOL;

typedef struct 
{
	UINT32 isValid;				/* 0~4: User's parameters valid flag. */

#define AT_MODE					(0)
#define DATA_MODE				(1)
	UINT8 atMode;				/* 5: 0: AT mode; 1:DATA mode*/

#define NORMAL_MODE				(0)
#define LIGHT_SLEEP_MODE		(1)
	UINT8 SleepMode;            /* 6: Sleep mode 0,normal; 1,light sleep */
	UINT32 Baudrate;  			/* 7~10: uart baudrate */

#define SOCK_UDP				(0)
#define SOCK_TCP				(1)	
	UINT8 socketProtocol;		/* 11: 0: UDP, 1:TCP */

#define SOCK_CLIENT				(0)
#define SOCK_SERVER				(1)
	UINT8 socketType;			/* 12: 0: client 1:server */

	UINT16 socketPort;			/* 13~14: socket port, default:8101 */
	UINT32 remoteCommIp;   		/* 15~18: remote ip address */
		    
	UINT16 frameLength;			/* 19~20: receive buffer size */
	UINT16 frameGap; 			/* 21~22: send gap time, default 10ms */
	
	CFG_PARAM cfg;    			/* 22-185   sizeof(CFG_PARAM) = 164 */
}USER_CFG_PARAM;

typedef struct
{
#define CONNECT_STATUS_FAIL		(0)
#define CONNECT_STATUS_OK		(1)
	UINT8 connStatus;				/* 0:without connect, 1:wifi connect */

	UINT8 socketProtocol;		/* 0:UDP, 1:TCP */
	UINT8 socketType;		    /* 0:client 1:server */
	UINT16 socketPort;			/* socket connect port */
	UINT32 remoteIp;			/* remote connect IP address */
	UINT32 localIp;			    /* local connect IP address */

	INT32 sockfd;				/* current socket file discript of connect, -1/0:no connect */
	INT32 connectFd;			/* current socket file discript of connected TCP client */

	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;

	OS_EVENT * modeSwitchSem;  	/* command and data mode switch */
	OS_EVENT * uartMessgSem;  	/* Uart error message print sem */

} CONN_STATUS; 


typedef struct
{
	UINT16 tx_status;
	UINT16 tx_port;
	UINT32 tx_len;
	INT32 tx_sockfd;

	UINT16 rx_status;
	UINT16 rx_port;
	UINT32 rx_len;
	INT32 rx_sockfd;

    struct sockaddr_in tx_addrto;

	struct sockaddr_in rx_server_addr;
	struct sockaddr_in rx_client_addr;
		
} UDP_BROADCAST_STS;

//#pragma pack()


extern UINT8 	uart_recvEnd;
extern UINT32	uart_rec_len;
extern char 	uart_rec_data[MAX_RECV_BUFFER_SIZE + 1];

extern UINT8 	net_sendStart;
extern UINT32	net_send_len;
extern char 	net_send_data[MAX_RECV_BUFFER_SIZE + 1];

extern UINT8 	uart_sendStart;				
extern UINT8	uart_hasData;				//0: 无数据发送 1:接收到数据
extern char 	uart_send_data[MAX_RECV_BUFFER_SIZE + 1];
extern UINT32	uart_send_len;
extern UINT8	end_sendFlags;
extern NST_TIMER *pTimer;
extern BOOL_T   isCancelled;
extern UINT8    ScanFlag;





extern SYS_EVT_ID link_status;
extern UINT8 smtconfigBegin;

extern UINT32 g_RecvBufSize;

extern USER_CFG_PARAM UserParam;
extern CFG_PARAM SysParam;

extern CONN_STATUS WifiConnStatus;

extern OS_EVENT *modeSwitchSem;
extern OS_EVENT *uartMessgSem;


							  
extern UDP_BROADCAST_STS broadcast_status;

/* Table for AP scan */
//extern AP_SCAN_TABLE ap_scan_table;


/**************************************************************************/
/******************* Common interface about serialNet *********************/
/**************************************************************************/


#endif
