
#ifndef __AT_CMD_H__
#define __AT_CMD_H__

#include "common.h"

/* Switch for UDP broadcast */
#define UDP_BROADCAST_SWITCH			(1)

/* AP scan table number */
#define AP_SCAN_TABLE_NUM 				(32)
											   
/* UART Frame length range */
#define UART_FRAME_MIN_LEN				(30)
#define UART_FRAME_MAX_LEN				(10000)


/* AT command request number */
typedef enum{
#define AT_BASE_NUM	  			(12)
	REQ_HELP = 0,
	REQ_VER = 1,
	REQ_SVER = 2,
	REQ_CHID = 3,
	REQ_SAVE = 4,
	REQ_FACTORY = 5,
	REQ_RST = 6,
	REQ_SYSTIME = 7,

	REQ_BAUDRATE = 10,
	REQ_UARTFT = 11,
	REQ_UARTFL = 12,

	REQ_MSLP = 15,
	REQ_LSLPT = 16,

#define AT_NET_NUM              (18)
	REQ_IPCONFIG = 20,
	REQ_PING = 21,
	REQ_MAC = 22,
	REQ_WQSOPT = 23,
	REQ_WPHYMODE = 24,
	REQ_WTXRATE = 25,
	REQ_WSCANAP = 26,
	REQ_WSBCN	= 27,

	REQ_WSCAP = 28,
	REQ_WSMTCONF = 29,
	REQ_WSTOP = 30,
	REQ_NQSCNN = 31,
	REQ_NLOCIP = 32,
	REQ_WSACONF = 33,
	REQ_AIRKISS = 34,

#if UDP_BROADCAST_SWITCH
	REQ_BCTTXSTART = 38,
	REQ_BCTTXSTOP = 39,
	REQ_BCTTXDATA = 40,
	REQ_BCTRXSTART = 41,
	REQ_BCTRXSTOP = 42,
#endif

#define AT_PASSTHROUGH    		(1)   
	REQ_QUIT = 60,
	
	REQ_MAX = 80
}REQUEST;

typedef struct{
		REQUEST requestNumber;
		char requestName[20];
		void (*responseFunction)(int argc ,char *argv[]);
}_S_AT_CMD;
 
/****************************************************************/
/***************** AT Command Process function *****************/
/****************************************************************/
static void responseHELP(int argc, char *argv[]);
static void responseVER(int argc, char *argv[]);
static void responseSVER(int argc, char *argv[]);
static void responseCHID(int argc, char *argv[]);
static void responseSAVE(int argc, char *argv[]);
static void responseFACTORY(int argc, char *argv[]);
static void responseRST(int argc, char *argv[]);
static void responseSYSTIME(int argc, char *argv[]);

static void responseBAUDRATE(int argc, char *argv[]);
static void responseUARTFT(int argc, char *argv[]);
static void responseUARTFL(int argc, char *argv[]);

static void responseMSLP(int argc, char *argv[]);
static void responseLSLPT(int argc, char *argv[]);

static void responseIPCONFIG(int argc, char *argv[]);
static void responsePING(int argc, char *argv[]);
static void responseMAC(int argc, char *argv[]);
static void responseWQSOPT(int argc, char *argv[]);
static void responseWPHYMODE(int argc, char *argv[]);
static void responseWTXRATE(int argc, char *argv[]);
static void responseWSCANAP(int argc, char *argv[]);
static void responseWSBCN(int argc, char *argv[]);


static void responseWSCAP(int argc, char *argv[]);
static void responseWSMTCONF(int argc, char *argv[]);
static void responseWSTOP(int argc, char *argv[]);

static void responseNQSCNN(int argc, char *argv[]);
static void responseNLOCIP(int argc, char *argv[]);
static void responseWSACONF(int argc, char *argv[]);
static void responseAIRKISS(int argc, char *argv[]);


#if UDP_BROADCAST_SWITCH
static void responseBCTTXSTART(int argc, char *argv[]);
static void responseBCTTXSTOP(int argc, char *argv[]);
static void responseBCTTXDATA(int argc, char *argv[]);
static void responseBCTRXSTART(int argc, char *argv[]);
static void responseBCTRXSTOP(int argc, char *argv[]);
#endif

static void responseQUIT(int argc, char *argv[]);


/* Check the input parameters string is digit */
int str_is_digit(const char * str);

/* check the validable of the IP string  */
int is_valid_ip(const char *ip);

/* 
 * Decompose a string into a set of strings 
 **/
char *strsep(char **stringp, const char *delim);

/* compare string between line with prefix.
 **/
int strStartsWith(const char *line, const char *prefix);

/* AT command process task thread.
 **/
VOID AtThread(VOID *arg);

/* Uart receive task thread
 **/							   
VOID UartRecvThread(VOID *arg);

/* init at command interface 
 **/
int init_atcommand_resource(void);

/*sofaware config WiFi
**/
VOID SoftApConfThread(VOID *arg);
VOID AirKissConfThread(VOID *arg);


#endif

