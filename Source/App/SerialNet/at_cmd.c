#include "at_cmd.h"
#include "airkiss.h"

#define AT_HEADER				"AT"
#define AT_HEADER_CMD			"AT+"
#define MAX_SINGLE_PARAMETER 	(50)

/* UDP Broadcast structure */
UDP_BROADCAST_STS broadcast_status;

typedef struct{
    UINT8 RATE_MODE;
    UINT8 RATE_IDX;
}RATE_OPT;

RATE_OPT rate_opt;

typedef struct{
    UINT32 BcnPrd;
    UINT8 NotifyPeerEn;
}BEACON_OPT;

BEACON_OPT beacon_opt ={100,0};


extern UINT8 smtconfigBegin;
extern CFG_PARAM SysParam;
extern NET_IF netif;

extern UINT8   PermanentAddress[MAC_ADDR_LEN];
extern const INT8U FwType[];
extern const INT8U FwVerNum[3];

const INT8U AtVerNum[3] = {
    0x01,  /* Main version */ 
    0x04,  /* Sub version */
    0x00   /* Internal version */
};



static _S_AT_CMD s_at_commands[] = {
    {REQ_HELP, 			"AT+HELP",  	responseHELP},
    {REQ_VER, 			"AT+VER", 		responseVER},
    {REQ_SVER, 			"AT+SVER", 		responseSVER},
    {REQ_SAVE, 			"AT+SAVE", 		responseSAVE},	
    {REQ_FACTORY, 		"AT+FACTORY", 	responseFACTORY},
    {REQ_RST, 			"AT+RST", 		responseRST}, 
	{REQ_SYSTIME,       "AT+SYSTIME",   responseSYSTIME},

    {REQ_BAUDRATE,		"AT+BAUDRATE",  responseBAUDRATE},
    {REQ_UARTFT,		"AT+UARTFT",  	responseUARTFT},
    {REQ_UARTFL,		"AT+UARTFL", 	responseUARTFL},

    {REQ_MSLP,			"AT+MSLP",  	responseMSLP},
    {REQ_LSLPT,			"AT+LSLPT", 	responseLSLPT},

    {REQ_IPCONFIG,	 	"AT+IPCONFIG", 	responseIPCONFIG},
    {REQ_PING,		 	"AT+PING",	 	responsePING},
    {REQ_MAC, 			"AT+MAC", 		responseMAC},
    {REQ_WQSOPT, 		"AT+WQSOPT", 	responseWQSOPT},
    {REQ_WPHYMODE,		"AT+WPHYMODE",	responseWPHYMODE},
    {REQ_WTXRATE, 		"AT+WTXRATE", 	responseWTXRATE},
    {REQ_WSCANAP, 		"AT+WSCANAP", 	responseWSCANAP},
    {REQ_WSBCN,		"AT+WSBCN",		responseWSBCN},

	{REQ_WSCAP, 		"AT+WSCAP", 	responseWSCAP},
	{REQ_WSMTCONF, 		"AT+WSMTCONF", 	responseWSMTCONF},
	{REQ_WSTOP, 		"AT+WSTOP", 	responseWSTOP},
	{REQ_NQSCNN,	"AT+NQSCNN", responseNQSCNN},
	{REQ_NLOCIP,		"AT+NLOCIP", 	responseNLOCIP},
	{REQ_WSACONF,	"AT+WSACONF",		responseWSACONF},
	{REQ_AIRKISS,	"AT+AIRKISS",		responseAIRKISS},

#if UDP_BROADCAST_SWITCH
    {REQ_BCTTXSTART,	"AT+BCTTXSTART", 	responseBCTTXSTART},
    {REQ_BCTTXSTOP,		"AT+BCTTXSTOP", 	responseBCTTXSTOP},
    {REQ_BCTTXDATA,		"AT+BCTTXDATA", 	responseBCTTXDATA},
    {REQ_BCTRXSTART,	"AT+BCTRXSTART", 	responseBCTRXSTART},
    {REQ_BCTRXSTOP,		"AT+BCTRXSTOP", 	responseBCTRXSTOP},
#endif

    {REQ_QUIT, 			"AT+QUIT", 		responseQUIT},	
};
#define AT_COMMAND_NUM (sizeof(s_at_commands) / sizeof(s_at_commands[0]))


/********************************************************************/
/**********************   External interface   **********************/
/********************************************************************/

extern VOID SaveUserParam(VOID);


/********************************************************************/

static int stringTorequest(char *str)
{
    UINT8 i;

    for (i = 0; i<strlen(str); i++)
        str[i] = toupper(str[i]);

    if (0 == strcmp(str, "HELP")) {
        return REQ_HELP;
    } else if (0 == strcmp(str, "VER")) {
        return REQ_VER;
    } else if(0 == strcmp(str, "SVER")) {
        return REQ_SVER;
    } else if (0 == strcmp(str, "SAVE")) {
        return REQ_SAVE;
    } else if (0 == strcmp(str, "FACTORY")) {
        return REQ_FACTORY;
    } else if (0 == strcmp(str, "RST")) {
        return REQ_RST;
    } else if (0 == strcmp(str, "SYSTIME")) {
        return REQ_SYSTIME;

    }else if (0 == strcmp(str, "BAUDRATE")) {
        return REQ_BAUDRATE;
    } else if (0 == strcmp(str, "UARTFT")) {
        return REQ_UARTFT;		
    } else if (0 == strcmp(str, "UARTFL")) {
        return REQ_UARTFL;

    } else if (0 == strcmp(str, "MSLP")) {
        return REQ_MSLP;
    } else if (0 == strcmp(str, "LSLPT")) {
        return REQ_LSLPT;


    } else if (0 == strcmp(str, "IPCONFIG")) {
        return REQ_IPCONFIG;
    } else if (0 == strcmp(str, "PING")) {
        return REQ_PING;
    } else if (0 == strcmp(str, "MAC")) {
        return REQ_MAC;
    } else if (0 == strcmp(str, "WQSOPT")) {
        return REQ_WQSOPT;
    } else if (0 == strcmp(str, "WSCANAP")) {
        return REQ_WSCANAP;
    } else if (0 == strcmp(str, "WSTOP")) {
        return REQ_WSTOP;
    } else if (0 == strcmp(str, "WPHYMODE")) {
        return REQ_WPHYMODE;
    } else if (0 == strcmp(str, "WTXRATE")) {
        return REQ_WTXRATE;
    } else if (0 == strcmp(str, "WSBCN")) {
        return REQ_WSBCN;



	} else if (0 == strcmp(str, "WSCAP")) {
		return REQ_WSCAP;
	} else if (0 == strcmp(str, "WSMTCONF")) {
		return REQ_WSMTCONF;
	} else if (0 == strcmp(str, "NQSCNN")) {
		return REQ_NQSCNN;	 
	} else if (0 == strcmp(str, "NLOCIP")) {
		return REQ_NLOCIP;
	} else if (0 == strcmp(str, "WSACONF")) {
		return REQ_WSACONF;	
	} else if (0 == strcmp(str, "AIRKISS")) {
	    return REQ_AIRKISS;	
#if UDP_BROADCAST_SWITCH
    } else if (0 == strcmp(str, "BCTTXSTART")) {
        return REQ_BCTTXSTART;
    } else if (0 == strcmp(str, "BCTTXSTOP")) {
        return REQ_BCTTXSTOP;
    } else if (0 == strcmp(str, "BCTTXDATA")) {
        return REQ_BCTTXDATA;
    } else if (0 == strcmp(str, "BCTRXSTART")) {
        return REQ_BCTRXSTART;
    } else if (0 == strcmp(str, "BCTRXSTOP")) {
        return REQ_BCTRXSTOP;
#endif

    } else if (0 == strcmp(str, "QUIT")) {
        return REQ_QUIT;
    }

    return INVALID_COMMAND;
}


/******************************************************************************/
/***********           Base AT command process interface           ************/
/******************************************************************************/
static void responseHELP(int argc ,char *argv[])
{
    UINT8 i;

    if (argc != 1) {
        printf("+ERROR=%d\n\r", INVALID_COMMAND);
        return;
    }

    printf("+++++++ COMMAND LIST ++++++\n");
    for(i = 0; i < AT_COMMAND_NUM; i++)
        printf("%s\n\r", s_at_commands[i].requestName);
    printf("------- COMMAND LIST ------\n+OK\n\r");
}


static void responseVER(int argc ,char *argv[])
{
    if (argc != 1) {
        printf("+ERROR=%d\n\r", INVALID_COMMAND);
        return;
    }

    printf("+OK=ATI:%d.%02x.%02x\n\r",
            AtVerNum[0], AtVerNum[1], AtVerNum[2]);
}


static void responseSVER(int argc ,char *argv[])
{
    if (argc != 1) {
        printf("+ERROR=%d\n\r", INVALID_COMMAND);
        return;
    }

    printf("+OK=%s:%x.%02x.%02x\n\r",
            FwType, FwVerNum[0], FwVerNum[1], FwVerNum[2]);

}

static void responseSAVE(int argc, char *argv[])
{
    if (argc != 1) {
        printf("+ERROR=%d\n\r", INVALID_COMMAND);
        return;
    }

    /* set user parameters valid flag */	
    UserParam.isValid = USER_PARAM_FLAG_VALID; 
    SaveUserParam();
    printf("+OK\n\r");
}

static void responseFACTORY(int argc, char *argv[])
{
    if (argc != 1) {
        printf("+ERROR=%d\n\r", INVALID_COMMAND);
        return;
    }

    memset((void*)&UserParam, 0, sizeof(USER_CFG_PARAM));
    SaveUserParam();

    printf("+OK:rebooting...\n\r");
    OSTimeDly(3);
    BSP_ChipReset();

}

static void responseRST(int argc, char *argv[])
{
    if (argc != 1) {
        printf("+ERROR=%d\n\r", INVALID_COMMAND);
        return;
    }

    printf("+OK:RST\n\r");
    OSTimeDly(3);
    BSP_ChipReset();
}

static void responseSYSTIME(int argc, char *argv[])
{
    UINT64 systimes = 0;

    if (argc != 1) {
        printf("+ERROR=%d\n\r", INVALID_COMMAND);
        return;
    }

	systimes = InfSysTimeGet();
    printf("+OK=SYSTIME:%d\n\r",(UINT32)systimes);
}

		

static void responseBAUDRATE(int argc, char *argv[])
{
    UINT32 Baudrate;

    if ((0 == strcmp(argv[1], "?")) && (argc == 2)) {
        printf("+OK=%d\n\r", UserParam.Baudrate);

    } else if ((argc == 2) && (str_is_digit(argv[1]))) {
        Baudrate = atoi(argv[1]);
        if (Baudrate == UART_BAUDRATE_300 || Baudrate == UART_BAUDRATE_1200
                || Baudrate == UART_BAUDRATE_2400 || Baudrate == UART_BAUDRATE_4800 || Baudrate == UART_BAUDRATE_9600
                || Baudrate == UART_BAUDRATE_19200 || Baudrate == UART_BAUDRATE_38400 || Baudrate == UART_BAUDRATE_57600
                || Baudrate == UART_BAUDRATE_115200 || Baudrate == UART_BAUDRATE_156250 || Baudrate == UART_BAUDRATE_250000
                || Baudrate == UART_BAUDRATE_312500 || Baudrate == UART_BAUDRATE_500000 || Baudrate == UART_BAUDRATE_625000 
                || Baudrate == UART_BAUDRATE_1250000)
        {
			NVIC_DisableIRQ(UART_IRQn);
            printf("+OK\n\r");
            /* you should delay few time befor or after seting uart baudrate */
            NSTusecDelay(200000);       
			BSP_UartOpen(Baudrate);
			NSTusecDelay(200000); 
            /* 
             * Save uart baudrate to user's parameter sector
             * */
            UserParam.Baudrate = Baudrate;
        } else {
            printf("+ERROR=%d\n\r", INVALID_PARAMETER);
        }
    } else {
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
    }
}

static void responseUARTFL(int argc, char *argv[])
{
    UINT16 recvBufSize;

    if ((argc == 2) && (0 == strcmp(argv[1], "?"))) {
        recvBufSize = UserParam.frameLength;
        printf("+OK=UARTFL:%d\n\r", recvBufSize);

    } else if ((argc == 2) && (str_is_digit(argv[1]))) {
        recvBufSize = atoi(argv[1]);
        if ((recvBufSize < MIN_RECV_BUFFER_SIZE)) {
            recvBufSize = MIN_RECV_BUFFER_SIZE;

        } else if ((recvBufSize > MAX_RECV_BUFFER_SIZE)) {
            recvBufSize = MAX_RECV_BUFFER_SIZE;
        }
        UserParam.frameLength = (recvBufSize & 0xffff);

        printf("+OK\n\r");

    } else {
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
    }
}

static void responseUARTFT(int argc, char *argv[])
{
    if ((argc == 2) && (0 == strcmp(argv[1], "?"))) {
        printf("+OK=UARTFT:%d\n\r", UserParam.frameGap);

    } else if((argc == 2) && ((str_is_digit(argv[1])))) {
        if (atoi(argv[1]) < UART_FRAME_MIN_LEN) {
            UserParam.frameGap = UART_FRAME_MIN_LEN;

        } else if (atoi(argv[1]) > UART_FRAME_MAX_LEN) {
            UserParam.frameGap = UART_FRAME_MAX_LEN;

        } else {
            UserParam.frameGap = atoi(argv[1]);
        }		
        printf("+OK\n\r");

    } else {
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
    }
}


static void responseMSLP(int argc, char *argv[])
{
    if (argc != 1) {
        printf("+ERROR=%d\n\r", INVALID_COMMAND);
        return;
    }

    if (UserParam.SleepMode == 0) {
        printf("+OK=NORMAL\n\r");	
    } else {
        printf("+OK=LIGHT\n\r");			
    }
}

static void responseLSLPT(int argc, char *argv[])
{
    UINT32 mode;
    UINT32 time;
    UINT32 DTIM;

    if ((argc != 4) 
            || (!str_is_digit(argv[1])) 
            || (!str_is_digit(argv[2]))
            || (!str_is_digit(argv[3]))
            || (strlen(argv[2]) > 3)) 
    {
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
        return;
    }

    mode = atoi(argv[1]);
    time = atoi(argv[2]);
    DTIM = atoi(argv[3]);


    if ((mode != 0 && mode != 1 && mode != 2 && mode != 3) || (time > 255 || time <= 0 ) || DTIM > 1 ) {
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
        return;
    }

    if((link_status !=SYS_EVT_LINK_UP) && (link_status != SYS_EVT_SCAN_DONE)){
        printf("+ERROR=WiFi isn't started\n\r");
        return;
    }

    UserParam.SleepMode = 1;	/* save sleep mode */	

    if (mode == 0) {
        InfPowerSaveSet(0);
        UserParam.SleepMode = 0;

    } else if (mode == 1) {	 	/* normal power save */
        if (time == 0) {		
            InfListenIntervalSet(0, DTIM);
        } else {				
            InfListenIntervalSet((UINT8)time, DTIM);
        }		
        InfPowerSaveSet(1);

    } else if (mode == 2) {	    /* quick power save */
        if (time == 0) {		
            InfListenIntervalSet(0, DTIM);
        } else {			
            InfListenIntervalSet((UINT8)time, DTIM);
        }		
        InfPowerSaveSet(2);			
    } else if (mode == 3) {	    /* Soft energy saving */
        if (time == 0) {		
            InfListenIntervalSet(0, DTIM);
        } else {			
            InfListenIntervalSet((UINT8)time, DTIM);
        }		
        InfPowerSaveSet(3);			
    }
	else{
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
        return;
    }
    printf("+OK\n\r");			
}


/******************************************************************************/
/***********        WIFI NET AT command process interface          ************/
/******************************************************************************/


static void responseIPCONFIG(int argc, char *argv[])
{
    int i;

    if (argc != 1) {
        printf("+ERROR=%d\n\r", INVALID_COMMAND);
        return;
    }

    printf("DevMode:%s\tHWaddr", 
            (UserParam.cfg.WiFiCfg.Protocol == 0) ?\
            "STA" : ((UserParam.cfg.WiFiCfg.Protocol == 2) ?\
                "SOFTAP" : "AHDOC"));
    for (i = 0; i < MAC_ADDR_LEN; i++)
        printf(":%02x", PermanentAddress[i]);

    printf("\r\nInet addr:%s    ", 
            inet_ntoa(netif.ip_addr.addr));
    printf("\r\nNetmask:%s    ", 
            inet_ntoa(netif.netmask.addr));
    printf("\r\nGateway:%s    ", 
            inet_ntoa(netif.gw.addr));

    if(SysParam.WiFiCfg.Protocol == 0)
        printf("\r\nChannel:%d,	",InfCurChGet());
    else
        printf("\r\nChannel:%d,	",SysParam.WiFiCfg.Channel);

    if(SysParam.WiFiCfg.Protocol == 2)
        printf("Encry:%s    ", 
                (SysParam.WiFiCfg.Encry== 0) ?\
                "NONE" : ((SysParam.WiFiCfg.Encry== 1) ?\
                    "WEP" : ((SysParam.WiFiCfg.Encry== 2) ?\
                        "TKIP": ((SysParam.WiFiCfg.Encry== 3) ?\
                            "CCMP": "AUTO"))));

    if(SysParam.WiFiCfg.Protocol == 2)
        printf("AuthMode:%s    ", 
                (SysParam.WiFiCfg.AuthMode== 0) ?\
                "OPEN" : ((SysParam.WiFiCfg.AuthMode== 1) ?\
                    "SHARE" : ((SysParam.WiFiCfg.AuthMode== 2) ?\
                        "WPA1": "WPA2")));

    printf("WMM:%s\n\r", (SysParam.WiFiCfg.WmmEn == 1) ? "ENABLE" : "DISABLE");

    printf("+OK\n\r");
}

static void responsePING(int argc, char *argv[])
{
    UINT32 remoteIp;
    int count;
    struct ip_addr dstipaddr;

    /* check the validable of IP string */
    if ((argc != 3) || (is_valid_ip(argv[1]) != 0) ||(str_is_digit(argv[2]) == 0) ||(strlen(argv[2]) == 0)) {
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
        return;		
    }

    inet_aton(argv[1], &remoteIp);
    count = atoi(argv[2]);
    dstipaddr.addr = remoteIp;


    if (ping_init(count) == 0) {
        ping_send_to(&dstipaddr, NULL);

    } else {
        printf("+ERROR=%d\n\r", INVALID_PING_RUNNING);
    }
}


static void responseMAC(int argc, char *argv[])
{
    UINT8 i;

    if (argc != 1) {
        printf("+ERROR=%d\n\r", INVALID_COMMAND);
        return;
    }

    printf("+OK=MAC");
    for (i = 0; i < MAC_ADDR_LEN; i++)
        printf(":%02x", PermanentAddress[i]);
    printf("\n\r");
}

static void responseWQSOPT(int argc, char *argv[])
{	
    if ((argc == 2) && (0 == strcmp(argv[1], "?"))){
        if (SysParam.WiFiCfg.Protocol == 0)
            printf("+OK=STA,");
        else if (SysParam.WiFiCfg.Protocol == 1)
            printf("+OK=ADHOC,");
        else if (SysParam.WiFiCfg.Protocol == 2)
            printf("+OK=SOFTAP,");
        else
            printf("+<mode>ERROR=%d,", INVALID_PARAMETER);

        if(SysParam.WiFiCfg.Protocol == 0)
            printf("%d,",InfCurChGet());
        else
            printf("%d,",SysParam.WiFiCfg.Channel);

        if (SysParam.WiFiCfg.Encry == 0) {
            printf("none,");
        } else if (SysParam.WiFiCfg.Encry == 1) {
            printf("wep,");
        } else if (SysParam.WiFiCfg.Encry == 2) {
            printf("tkip,");
        } else if (SysParam.WiFiCfg.Encry == 3) {
            printf("ccmp,");
        } else if (SysParam.WiFiCfg.Encry == 4) {
		    if(SysParam.WiFiCfg.Protocol == 2){
			   printf("none,");		
			} else {
               printf("auto,");			
			}
        }
        else {
            printf("+ERROR=%d\n\r", INVALID_COMMAND);
        }

        if (SysParam.WiFiCfg.AuthMode == 0) {
            printf("open,");
        } else if (SysParam.WiFiCfg.AuthMode == 1) {
            printf("share,");
        } else if (SysParam.WiFiCfg.AuthMode == 2) {
            printf("wpa,");
        } else if (SysParam.WiFiCfg.AuthMode == 3) {
            printf("wpa2,");
        } else {
            printf("+ERROR=%d\n\r", INVALID_COMMAND);
        }

        if (SysParam.WiFiCfg.WmmEn == 0) {
            printf("Disable\n\r");
        } else if (SysParam.WiFiCfg.WmmEn == 1) {
            printf("Enable\n\r");
        }		

        return;
    }

    if((argc == 6)){
        UINT8 mode,channel,encry,AuthMode,WmmEn;
        mode = (UINT8)atoi(argv[1]);

        if(mode == PARAM_NET_MODE_STA_BSS)
            channel = 1;
        else
            channel = (UINT8)atoi(argv[2]);

        encry = (UINT8)atoi(argv[3]);
        AuthMode = (UINT8)atoi(argv[4]);
        WmmEn = (UINT8)atoi(argv[5]);

        if((str_is_digit(argv[1])) && (strlen(argv[1]) == 1)
                &&(str_is_digit(argv[2])) && (strlen(argv[2]) <= 2)
                &&(str_is_digit(argv[3])) && (strlen(argv[3]) == 1)
                &&(str_is_digit(argv[4])) && (strlen(argv[4]) == 1)
                &&(str_is_digit(argv[5])) && (strlen(argv[5]) == 1)){

            if (((mode != PARAM_NET_MODE_STA_BSS) 
                        && (mode != PARAM_NET_MODE_STA_ADHOC) 
                        && (mode != PARAM_NET_MODE_SOFTAP))
                    ||(channel < 1 || channel > 13)
                    ||(encry > 4)
                    ||(AuthMode > 3)
                    ||(WmmEn != 0 && WmmEn != 1)) {
                printf("+ERROR=%d\n\r", INVALID_PARAMETER);
                return;

            } else {
                if((link_status ==SYS_EVT_LINK_UP) || (link_status == SYS_EVT_SCAN_DONE)){
				  	InfWiFiStop();
                	OSTimeDly(10);  
                    //printf("+ERROR=WiFi Started\n\r");
                    //return;
                }

                if(encry == 0 && AuthMode != 0){
                    printf("+ERROR=%d\n\r", INVALID_PARAMETER);
                    return;
                }

                if(mode== PARAM_NET_MODE_SOFTAP && encry == 4 && AuthMode != 0){
                    printf("+ERROR=%d\n\r", INVALID_PARAMETER);
                    return;
                }

                InfNetModeSet((PARAM_NET_MODE)mode);  //set net mode
                SysParam.WiFiCfg.Channel = channel;
                SysParam.WiFiCfg.Encry = encry;
                SysParam.WiFiCfg.AuthMode = AuthMode;
                SysParam.WiFiCfg.WmmEn = WmmEn;
                memcpy(&UserParam.cfg, &SysParam, sizeof(CFG_PARAM));

                printf("+OK\r\n");
                return;
            }

        }else{
            printf("+ERROR=%d\n\r", INVALID_PARAMETER);
            return;
        }
    }else{
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
        return;
    }
}

static void responseWTXRATE(int argc, char *argv[])
{
    UINT8 mode = 0,rate = 0;
    char *mode_cck_rate[10] = {"auto", "1Mbps", "2Mbps" ,"5.5Mbps", "11Mbps"};
    char *mode_ofdm_rate[10] = {"auto", "6Mbps","9Mbps","12Mbps","18Mbps","24Mbps","36Mbps","48Mbps","54Mbps"};
    //char *mode_ht_rate[10] = {"auto","Mcs0", "Mcs1", "Mcs2", "Mcs3", "Mcs4", "Mcs5", "Mcs6", "Mcs7"};

    if ((argc == 2) && (0 == strcmp(argv[1], "?"))) {
        if (rate_opt.RATE_MODE== 0){
            printf("+OK=%s\n\r",mode_cck_rate[rate_opt.RATE_IDX]);
        }
        else if (rate_opt.RATE_MODE== 1){
            printf("+OK=%s\n\r",mode_ofdm_rate[rate_opt.RATE_IDX]);
        }else
            printf("+ERROR=%d\n\r", INVALID_PARAMETER);

        return;
    }

    if (argc == 2 && (strcmp(argv[1],"0") == 0
                ||strcmp(argv[1],"1") == 0 ||strcmp(argv[1],"2") == 0
                ||strcmp(argv[1],"5.5") == 0 || strcmp(argv[1],"11") == 0
                ||strcmp(argv[1],"6") == 0 ||strcmp(argv[1],"9") == 0
                ||strcmp(argv[1],"12") == 0 ||strcmp(argv[1],"18") == 0
                ||strcmp(argv[1],"24") == 0 ||strcmp(argv[1],"36") == 0
                ||strcmp(argv[1],"48") == 0 ||strcmp(argv[1],"54") == 0)) {

        if((link_status != SYS_EVT_LINK_UP)&& 
                (link_status != SYS_EVT_SCAN_DONE)){
            printf("+ERROR=WiFi isn't started\n\r");
            return;
        }

        if(strcmp(argv[1],"0") == 0){
            mode = 0;
            rate = 0;
        }else if(strcmp(argv[1],"1") == 0){
            mode = 0;
            rate = 1;
        }else if(strcmp(argv[1],"2") == 0){
            mode = 0;
            rate = 2;
        }else if(strcmp(argv[1],"5.5") == 0){
            mode = 0;
            rate = 3;
        }else if(strcmp(argv[1],"11") == 0){
            mode = 0;
            rate = 4;
        }else if(strcmp(argv[1],"6") == 0){
            mode = 1;
            rate = 1;
        }else if(strcmp(argv[1],"9") == 0){
            mode = 1;
            rate = 2;
        }else if(strcmp(argv[1],"12") == 0){
            mode = 1;
            rate = 3;
        }else if(strcmp(argv[1],"18") == 0){
            mode = 1;
            rate = 4;
        }else if(strcmp(argv[1],"24") == 0){
            mode = 1;
            rate = 5;
        }else if(strcmp(argv[1],"36") == 0){
            mode = 1;
            rate = 6;
        }else if(strcmp(argv[1],"48") == 0){
            mode = 1;
            rate = 7;
        }else if(strcmp(argv[1],"54") == 0){
            mode = 1;
            rate = 8;
        }

        if(InfTxRateSet((PARAM_RATE_MODE)mode,(PARAM_RATE_IDX)rate)!= 0)
        {
            printf("+ERROR=%d\n\r", INVALID_PARAMETER);
            return;
        }  //set  Txrate

        rate_opt.RATE_MODE= mode;
        rate_opt.RATE_IDX = rate;
        printf("+OK\n\r");
    } else {
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
    }

}

static void responseWSCANAP(int argc, char *argv[])
{
    int ret;
    int i,count;
    INT8 MinRssiFilter;
    SCAN_INFO_TABLE * pScanTable = NULL;

    /* Malloc the scan table memory */
    pScanTable = OSMMalloc(sizeof(SCAN_INFO_TABLE) + sizeof(SCAN_INFO) * (AP_SCAN_TABLE_NUM - 1));
    if (!pScanTable) {
        printf("+ERROR=%d\n\r", INVALID_SCAN_AP_FAIL);
        return;
    }

    if ((argc == 2) && (str_is_digit(argv[1])) && (strlen(argv[1]) != 0)) {
        MinRssiFilter = (UINT8)atoi(argv[1]);
        if(MinRssiFilter > 127)
        {
            printf("+ERROR=%d\n\r", INVALID_PARAMETER);
            return;
        }

        //		printf("rssifilter:%d\n\r", -MinRssiFilter);
        for(i = 0; i <  AP_SCAN_TABLE_NUM; i++){
            memset(&(pScanTable->InfoList[i].Ssid),
                    0, sizeof(pScanTable->InfoList[i].Ssid));
        }

        pScanTable->InfoCount = AP_SCAN_TABLE_NUM;
        ScanFlag = 0;

        printf("+OK:scanning...\n\r");
        if(link_status != SYS_EVT_LINK_UP && link_status != SYS_EVT_SCAN_DONE){
            InfSsidSet(NULL,0);
            InfWiFiStart();
        }

        ret = InfScanStart(pScanTable, -MinRssiFilter, 0);
        if (ret != 0) {
            printf("+ERROR=%d\n\r", INVALID_SCAN_AP_FAIL);
            return;

        } else {
            while(ScanFlag != 1)
                OSTimeDly(10);

            if(strlen(&SysParam.WiFiCfg.Ssid) == 0){
                InfWiFiStop();
            }

            count = pScanTable->InfoCount; 
            printf("Scan number:%d\n\r", pScanTable->InfoCount);
            printf("+++++++ SCAN TABLE +++++++\n\r");

            for (i = 0; i < count; i++) { 
                printf("Ssid:%s, Channel:%d, EncryMode:%d, AuthMode:%d, Rssi:%d.\n\r",  
                        pScanTable->InfoList[i].Ssid, 
                        pScanTable->InfoList[i].Channel,
                        pScanTable->InfoList[i].EncryMode,
                        pScanTable->InfoList[i].AuthMode,
                        pScanTable->InfoList[i].Rssi); 
            } 
            printf("+++++++ SCAN TABLE +++++++\n\r"); 

            OSMFree(pScanTable);	
            printf("+OK:ScanFinish\n\r");
        }

    } else {
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
    }
}

static void responseWSBCN(int argc, char *argv[])
{
    UINT32 period;
    UINT32 mode;

    if ((argc == 2) && (0 == strcmp(argv[1], "?"))) {
        if (beacon_opt.NotifyPeerEn== 0){
            printf("+OK=Period:%d  ",beacon_opt.BcnPrd);
            printf("NotifyPeerEn:DISANBLE\n\r");
        }
        else if (beacon_opt.NotifyPeerEn== 1){
            printf("+OK=Period:%d  ",beacon_opt.BcnPrd);
            printf("NotifyPeerEn:ENABLE\n\r");
        }
        else
            printf("+ERROR=%d\n\r", INVALID_PARAMETER);

        return;
    } 

    if ((argc == 3) && (str_is_digit(argv[1])) 
            && (str_is_digit(argv[2])) && (strlen(argv[2]) == 1)) {

        if(SysParam.WiFiCfg.Protocol == 0){
            printf("+ERROR=%d\n\r", INVALID_PARAMETER);
            return;
        }

        period = atoi(argv[1]);
        mode = (UINT8)atoi(argv[2]);

        if (period < 100 || period > 65535|| (mode != 0 && mode != 1)) {
            printf("+ERROR=%d\n\r", INVALID_PARAMETER);
            return;
        }else {	
            if(InfBeaconPeriodSet((UINT16)period,mode) != 0)
            {
                printf("+ERROR=%d\n\r", INVALID_PARAMETER);
                return;
            }

            if((link_status != SYS_EVT_LINK_UP) && (link_status != SYS_EVT_SCAN_DONE)){
                printf("+ERROR=WiFi isn't started\n\r");
                return;
            }

            beacon_opt.BcnPrd= (UINT16)period;
            beacon_opt.NotifyPeerEn= mode;
            printf("+OK\n\r");

        }
    } else {
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
    }

}


static void responseWSCAP(int argc, char *argv[])
{
    //	IP_CFG IpParam;
    UINT32 Trytime;

    if ((argc == 2) && (0 == strcmp(argv[1], "?"))) {
        SysParam.WiFiCfg.PSK[SysParam.WiFiCfg.KeyLength] = '\0';
        printf("+OK=%s,%s\n\r", SysParam.WiFiCfg.Ssid, SysParam.WiFiCfg.PSK);

    } else if (argc == 4 && (strlen(argv[1]) <= 32 && strlen(argv[1]) > 0)
            && strlen(argv[2]) <= 64) {

        if (SysParam.WiFiCfg.Protocol == 0) {

            if (str_is_digit(argv[3]) == 0 || strlen(argv[3]) == 0) {
                printf("+ERROR=%d\n\r", INVALID_PARAMETER);
                return;	
            }

            Trytime = atoi(argv[3]);
            if(Trytime > 255){
                printf("+ERROR=%d\n\r", INVALID_PARAMETER);
                return;	
            }

            InfWiFiStop();
            OSTimeDly(10);

            //			InfLoadDefaultParam();		
            //			memcpy(&SysParam, &UserParam.cfg, sizeof(CFG_PARAM));					
            //			memset(&IpParam, 0, sizeof(IpParam));
            //			InfIpSet(&IpParam);

            InfNetModeSet(PARAM_NET_MODE_STA_BSS); 
            InfSsidSet((UINT8 *)argv[1], strlen(argv[1]));    /* set ssid */
            InfKeySet(0, (UINT8 *)argv[2], strlen(argv[2]));  /* set pw */
            InfConTryTimesSet((UINT8)Trytime);                       /* set trytime */	

            printf("+OK:connecting...\n\r");
            InfWiFiStart();
            memcpy(&UserParam.cfg, &SysParam, sizeof(CFG_PARAM));	

        } else if (SysParam.WiFiCfg.Protocol == 2) {
            InfWiFiStop();
            OSTimeDly(10);

            memcpy(&SysParam, &UserParam.cfg, sizeof(CFG_PARAM));
            SysParam.bDhcpServer = 1;
            SysParam.bDnsServer = 1;
            InfNetModeSet(PARAM_NET_MODE_SOFTAP); 

            InfSsidSet((UINT8 *)argv[1], strlen(argv[1]));    /* set ssid */
            InfKeySet(0, (UINT8 *)argv[2], strlen(argv[2]));  /* set pw */

            printf("+OK:setting...\n\r");
            InfWiFiStart();			
            memcpy(&UserParam.cfg, &SysParam, sizeof(CFG_PARAM));					
        }

    } else {
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
    }
}

static void responseWSMTCONF(int argc, char *argv[])
{
    if (argc != 1) {
        printf("+ERROR=%d\n\r", INVALID_COMMAND);
        return;
    }

    if (smtconfigBegin == 0) {		
        InfWiFiStop();
        OSTimeDly(10);

        //		InfLoadDefaultParam();
        InfNetModeSet(PARAM_NET_MODE_STA_BSS);
        InfDirectCfgStart(0, 0, 0);
        smtconfigBegin = 1;
        printf("+OK:DirectConfig...\n\r");

    } else {
        printf("+ERROR=%d\n\r", INVALID_SMTCONF_RUNNING);
    }
}

static void responseWSTOP(int argc, char *argv[])
{

    if (argc != 1) {
        printf("+ERROR=%d\n\r", INVALID_COMMAND);
        return;
    }

    if((link_status != SYS_EVT_LINK_UP) && (link_status != SYS_EVT_SCAN_DONE)){
        printf("+ERROR=WiFi isn't started\n\r");
        return;
    }

    InfWiFiStop();
    printf("+OK:WiFi Stopped\n\r");
}

static void responseWPHYMODE(int argc, char *argv[])
{
    UINT32 mode;


    if ((argc == 2) && (0 == strcmp(argv[1], "?"))) {
        if (SysParam.WiFiCfg.PhyMode== 1)
            printf("+OK=PhyMode:b/g\n\r");
        else if (SysParam.WiFiCfg.PhyMode== 2)
            printf("+OK=PhyMode:b\n\r");
        else
            printf("+ERROR=%d\n\r", INVALID_PARAMETER);

        return;
    }

    if ((argc == 2) && (str_is_digit(argv[1])) && (strlen(argv[1]) == 1)) {
        mode = atoi(argv[1]);
        if ((mode != PARAM_PHY_MODE_BG) && (mode != PARAM_PHY_MODE_B_ONLY)) 
        {
            printf("+ERROR=%d\n\r", INVALID_PARAMETER);
            return;
        } else {
            if((link_status == SYS_EVT_LINK_UP) || (link_status == SYS_EVT_SCAN_DONE)){
                printf("+ERROR=WiFi Started\n\r");
                return;
            }
            UserParam.cfg.WiFiCfg.PhyMode = mode;							
            InfPhyModeSet((PARAM_PHY_MODE)mode);  //set phy mode
            memcpy(&UserParam.cfg, &SysParam, sizeof(CFG_PARAM));
            printf("+OK\n\r");
        }

    } else {
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
    }


}


static int responseNMODE(int argc, char *argv[])
{
    UINT8 protocol;

    if ((argc == 2) && (str_is_digit(argv[1])) && (strlen(argv[1]) == 1)) {
        protocol = (UINT8)atoi(argv[1]);
        if (protocol != 0 && protocol != 1) {
            return -1;

        } else {
            UserParam.socketProtocol = protocol;
            memcpy(&UserParam.cfg, &SysParam, sizeof(CFG_PARAM));
            return 0;
        }
    }else
        return -1;
}

static int responseNTYPE(int argc, char *argv[])
{
    UINT8 nettype;

    if ((argc == 2) && (str_is_digit(argv[1])) && (strlen(argv[1]) == 1)) {
        nettype = (UINT8)atoi(argv[1]);
        if (nettype != 0 && nettype != 1) {
            return -1;
        } else {
            UserParam.socketType = nettype;
            memcpy(&UserParam.cfg, &SysParam, sizeof(CFG_PARAM));
            return 0;
        }

    }else
        return -1;
}

static int responseNPORT(int argc, char *argv[])
{
    UINT16 socketPort;

    if ((argc == 2) && (str_is_digit(argv[1]))) {		
        if ((atoi(argv[1]) > 65535) || (atoi(argv[1]) == 0)) {
            return -1;
        }else{
            socketPort = atoi(argv[1]);
            UserParam.socketPort = socketPort;

            memcpy(&UserParam.cfg, &SysParam, sizeof(CFG_PARAM));
            return 0;
        }
    } else
        return -1;
}

static int responseNRMTIP(int argc, char *argv[])
{
    UINT32 remoteIp;

    if((argc == 2) && (UserParam.socketType == 1))
        return 0;

    if ((argc == 2) && (is_valid_ip(argv[1]) == 0)) {
        /* check the validable of IP string */

        inet_aton(argv[1], &remoteIp);
        UserParam.remoteCommIp = remoteIp;
        memcpy(&UserParam.cfg, &SysParam, sizeof(CFG_PARAM));

        return 0;
    }else
        return -1;
}

static void responseNQSCNN(int argc, char *argv[])
{
    int count = 0;

    if ((argc == 2) && (0 == strcmp(argv[1], "?"))) {
        if (UserParam.socketProtocol == 0) {
            printf("+OK=UDP,");
        } else if (UserParam.socketProtocol == 1) {
            printf("+OK=TCP,");
        }

        if (UserParam.socketType == 1) {
            printf("server,");
        } else if (UserParam.socketType == 0) {
            printf("client,");
        } 

        printf("%d,", UserParam.socketPort);

        printf("%s\n\r", inet_ntoa(UserParam.remoteCommIp));
    }else if(argc == 5){

        if(responseNMODE(2,argv++) == 0)
            count++;
        if(responseNTYPE(2,argv++) == 0)
            count++;
        if(responseNPORT(2,argv++) == 0)
            count++;
        if(responseNRMTIP(2,argv) == 0)
            count++;

        if(count == 4)
        {
            printf("+OK\n\r");
        }else{
            printf("+ERROR=%d\n\r", INVALID_PARAMETER);
        }

    }else{
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
    }

}


static void responseNLOCIP(int argc, char *argv[])
{
    ip_addr_t ipAddr;
    UINT8 mode;
    UINT8 trytimes;

    if ((argc == 2) && (0 == strcmp(argv[1], "?"))) {
        if((link_status ==SYS_EVT_LINK_UP) || (link_status == SYS_EVT_SCAN_DONE))
            printf("\r\nInet addr:%s    ", inet_ntoa(netif.ip_addr.addr));
        else
            printf("+OK=IP:%d.%d.%d.%d\n\r",
                    SysParam.IPCfg.Ip[0],
                    SysParam.IPCfg.Ip[1],
                    SysParam.IPCfg.Ip[2],
                    SysParam.IPCfg.Ip[3]);

        if(SysParam.IPCfg.bDhcp == 0)
            printf("dhcp:disable");
        if(SysParam.IPCfg.bDhcp == 1)
            printf("dhcp:enable");
        printf(" trytimes:%d\n\r",SysParam.IPCfg.DhcpTryTimes);
        return;

    }

    if ((argc == 4)) {
        if (is_valid_ip(argv[1]) != 0  ) {

            printf("+ERROR=%d\n\r", INVALID_PARAMETER);
            return;

        } else {
            mode = (UINT8)atoi(argv[2]);
            trytimes = (UINT8)atoi(argv[3]);
            if((str_is_digit(argv[2]) == 0) || (strlen(argv[2]) == 0) ||
                    (str_is_digit(argv[3]) == 0) || (strlen(argv[3]) == 0) ||
                    (mode != 0 && mode != 1) ){
                printf("+ERROR=%d\n\r", INVALID_PARAMETER);
                return;
            }

            if((link_status ==SYS_EVT_LINK_UP) || (link_status == SYS_EVT_SCAN_DONE)){
                printf("+ERROR=WiFi Started\n\r");
                return;
            }

            inet_aton(argv[1], &ipAddr);
            SysParam.IPCfg.Ip[0] = ip4_addr1(&ipAddr);
            SysParam.IPCfg.Ip[1] = ip4_addr2(&ipAddr);
            SysParam.IPCfg.Ip[2] = ip4_addr3(&ipAddr);
            SysParam.IPCfg.Ip[3] = ip4_addr4(&ipAddr);

            SysParam.IPCfg.GateWay[0] = ip4_addr1(&ipAddr);
            SysParam.IPCfg.GateWay[1] = ip4_addr2(&ipAddr);
            SysParam.IPCfg.GateWay[2] = ip4_addr3(&ipAddr);
            SysParam.IPCfg.GateWay[3] = ip4_addr4(&ipAddr);

            if(SysParam.WiFiCfg.Protocol == 0)
            {
                SysParam.IPCfg.bDhcp = mode;
                SysParam.IPCfg.DhcpTryTimes = trytimes;
            }

            memcpy(&UserParam.cfg, &SysParam, sizeof(CFG_PARAM));

            printf("+OK\n\r");
        }
    } else {
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
    }
}

static void responseWSACONF(int argc, char *argv[])
{	
	if(argc == 1){
		sys_thread_new("SoftApConfThread",SoftApConfThread, NULL, NST_TEST_APP_TASK_STK_SIZE, TCPIP_THREAD_PRIO+8);
	}else{
		printf("+ERROR=%d\r\n", INVALID_PARAMETER);
	}
}


static void responseAIRKISS(int argc, char *argv[])
{	
	if(argc == 1){
		sys_thread_new("AirKissConfThread",AirKissConfThread, NULL, NST_TEST_APP_TASK_STK_SIZE, TCPIP_THREAD_PRIO+1);
	}else{
		printf("+ERROR=%d\r\n", INVALID_PARAMETER);
	}
}


#if UDP_BROADCAST_SWITCH

static void responseBCTTXSTART(int argc, char *argv[])
{
    /* create UDP broadcast socket */
    int opt = 1;

    if ((argc == 2) && (str_is_digit(argv[1]))) {		
        if (atoi(argv[1]) > 65535 || atoi(argv[1]) == 0) {
            printf("+ERROR=%d\n\r", INVALID_PARAMETER);
            return;
        }

        //		printf("prot:%d.\n\r", broadcast_status.tx_port);

        if ((link_status != SYS_EVT_LINK_UP) && (link_status != SYS_EVT_SCAN_DONE)) {
            printf("+ERROR=link_status isn't SYS_EVT_LINK_UP\n\r");
            return;
        } 

        if(broadcast_status.rx_status != 0){
            printf("+ERROR=%d\n\r",INVALID_BCTRX_RUNNING_OR_LINKED);
            return;
        }

        if(broadcast_status.tx_status != 0){
            printf("+ERROR=%d\n\r",INVALID_BCTTX_RUNNING_OR_LINKED);
            return;
        }

        broadcast_status.tx_port = atoi(argv[1]);

        if ((broadcast_status.tx_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
            printf("+ERROR= %d\n\r", INVALID_BCTTX_SOCK);
            return;
        }

        if (setsockopt(broadcast_status.tx_sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt)) == -1) {  
            printf("+ERROR= %d\n\r", INVALID_BCTTX_SETOPT);  
            lwip_close(broadcast_status.tx_sockfd);
            return;  
        }

        memset(&broadcast_status.tx_addrto, 0, sizeof(struct sockaddr_in));  
        broadcast_status.tx_addrto.sin_family = AF_INET;  
        broadcast_status.tx_addrto.sin_addr.s_addr = htonl(INADDR_BROADCAST);  
        broadcast_status.tx_addrto.sin_port = htons(broadcast_status.tx_port);

        broadcast_status.tx_status = 1;

        printf("+OK:BCTTXSTART\n\r");

    } else {
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
    }
}

static void responseBCTTXSTOP(int argc, char *argv[])
{
    if (argc != 1) {
        printf("+ERROR=%d\n\r", INVALID_COMMAND);
        return;
    }

    broadcast_status.tx_status = 0;
    OSTimeDly(20);

    if (broadcast_status.tx_sockfd >= 0) {
        lwip_close(broadcast_status.tx_sockfd);
    }
    printf("+OK:BCTTXSTOP\n\r");
}

static void responseBCTTXDATA(int argc, char *argv[])
{
    int ret;
    int nlen = sizeof(broadcast_status.tx_addrto);

    if ((argc == 3) && (str_is_digit(argv[1]))) {

        broadcast_status.tx_len = atoi(argv[1]);
        if (broadcast_status.tx_len > 256 || broadcast_status.tx_len == 0) {
            broadcast_status.tx_len = 0;
            printf("+ERROR=%d\n\r", INVALID_PARAMETER);
            return;
        }

        if (link_status != SYS_EVT_LINK_UP && (link_status != SYS_EVT_SCAN_DONE)) {
            printf("+ERROR=link_status isn't SYS_EVT_LINK_UP\n\r");
            return;		
        }

        if (broadcast_status.tx_sockfd < 0) {
            printf("+ERROR= %d\n\r", INVALID_BCTTX_SOCK);
            return;
        }

        ret = sendto(broadcast_status.tx_sockfd, argv[2], 
                broadcast_status.tx_len, 0, (struct sockaddr *)&broadcast_status.tx_addrto, nlen);
        if (ret < 0) {
            printf("+ERROR=%d\n\r", INVALID_BCTTX_SEND);
        }else
            printf("+OK\n\r");

    } else {
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
    }
}

static void responseBCTRXSTART(int argc, char *argv[])
{
    if ((argc == 2) && (str_is_digit(argv[1]))) {		
        if (atoi(argv[1]) > 65535 || atoi(argv[1]) == 0) {
            printf("+ERROR=%d\n\r", INVALID_PARAMETER);
            return;
        }

        if ((link_status != SYS_EVT_LINK_UP) && (link_status != SYS_EVT_SCAN_DONE)) {

            printf("+ERROR=link_status isn't SYS_EVT_LINK_UP\n\r");
            return;
        }

        if(broadcast_status.tx_status != 0){
            printf("+ERROR=%d\n\r",INVALID_BCTTX_RUNNING_OR_LINKED);
            return;
        }

        if(broadcast_status.rx_status != 0){
            printf("+ERROR=%d\n\r",INVALID_BCTRX_RUNNING_OR_LINKED);
            return;
        }

        broadcast_status.rx_port = atoi(argv[1]);

        if ((broadcast_status.rx_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
            printf("+ERROR=%d\n\r", INVALID_BCTRX_SOCK);
            return;
        }

        memset(&broadcast_status.rx_server_addr, 0, sizeof(struct sockaddr_in));  
        broadcast_status.rx_server_addr.sin_family = AF_INET;  
        broadcast_status.rx_server_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST); 
        broadcast_status.rx_server_addr.sin_port = htons(broadcast_status.rx_port);

        if (bind(broadcast_status.rx_sockfd, 
                    (struct sockaddr *)&broadcast_status.rx_server_addr, 
                    sizeof(struct sockaddr)) == -1) {
            printf("+ERROR=%d\n\r", INVALID_BCTRX_BIND);
            return;
        }

        //		fcntl(broadcast_status.rx_sockfd, F_SETFL, 0); 	/* set block */

        /* Start to receive UDP broadcast data. */
        broadcast_status.rx_status = 1;
        printf("+OK:BCTRXSTART\n\r");
    }else{
        printf("+ERROR=%d\n\r", INVALID_PARAMETER);
        return;
    }
}

static void responseBCTRXSTOP(int argc, char *argv[])
{
    if (argc != 1) {
        printf("+ERROR=%d\n\r", INVALID_COMMAND);
        return;
    }

    broadcast_status.rx_status = 0;
    OSTimeDly(20);

    if (broadcast_status.rx_sockfd >= 0) {
        lwip_close(broadcast_status.rx_sockfd);
    }
    printf("+OK:BCTRXSTOP\n\r");
}
#endif


static void responseQUIT(int argc ,char *argv[])
{
    if (argc != 1) {
        printf("+ERROR=%d\n\r", INVALID_COMMAND);
        return;
    }

    if (link_status != SYS_EVT_LINK_UP && link_status != SYS_EVT_SCAN_DONE) {
        printf("+ERROR=%d\n\r", INVALID_DATA_LINKDOWN);
        return;
    }

    UserParam.atMode = DATA_MODE;

    /* Reset net send/receive viable */
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

    printf("+OK:DATAMODE\n");
}


static void processCommand(int argc, char *argv[])
{
    int i;
    int request;
    if (argc >= 1) {
        request = stringTorequest(argv[0]);

        /* get MAC address from AT command list */
        for (i = 0; i < AT_COMMAND_NUM; i++) {
            if (request == s_at_commands[i].requestNumber) {
                (&(s_at_commands[i]))->responseFunction(argc, argv);
                break;
            }
        }

        if (i >= AT_COMMAND_NUM) {
            printf("+ERROR= %d\n\r", INVALID_COMMAND);
        }

    } else {
        printf("+ERROR=%d\n\r", INVALID_COMMAND);
    }
}


static void parseParameter(char *args)
{
    int argc = 1;
    char *p = args;
    char *argv[MAX_SINGLE_PARAMETER];

    argv[0] = strsep(&p, "=");
    while (p) {
        while ((argv[argc] = strsep(&p, ",")) != NULL) {
            argc++;
        }
    }
    processCommand(argc, argv);
}


VOID AtThread(VOID *arg)
{

#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0u;
#endif

    printf("SDK Version:<%x.%02x.%02x>; AT Version:<%x.%02x.%02x> Release:<%s>\n\r", 
            FwVerNum[0], FwVerNum[1], FwVerNum[2],
            AtVerNum[0], AtVerNum[1], AtVerNum[2],
#if DEBUG_ON
            "dbg"
#else
            "rel"
#endif
          );

    while (1) {
		if (UserParam.atMode == AT_MODE) {
			if (uart_recvEnd == 1) {
 
			    uart_rec_data[0] = toupper(uart_rec_data[0]);
			    uart_rec_data[1] = toupper(uart_rec_data[1]);

				if (strStartsWith(uart_rec_data, AT_HEADER) == 1) {	
					if (0 == strcmp(uart_rec_data, AT_HEADER)) {	/* AT test command */
						printf("+OK\n\r");

					} else {	 /* AT command with parameters */
						if (strStartsWith(uart_rec_data, AT_HEADER_CMD) == 1) {
							parseParameter(&uart_rec_data[3]);

						} else {
							printf("+ERROR= %d\n\r", INVALID_COMMAND);
						}
					}

				} else {
					printf("+ERROR= %d\n\r", INVALID_COMMAND);
				}

                OS_ENTER_CRITICAL();
				uart_rec_len = 0;
			    uart_recvEnd = 0;
		  		memset(uart_rec_data, '\0', MAX_RECV_BUFFER_SIZE);
				OS_EXIT_CRITICAL();
			} else {
				OSTimeDly(20);
			}
		} else {
			OSTimeDly(100);
		}
	}
}

/*
 * software timer for copying data from uart send buffer to net buffer. 
 **/
static void timerFuncProcess(void *ptmr, void *parg)
{
	if (net_sendStart == 1) {
		return ;
	} else if (uart_rec_len != 0) {
		net_send_len = uart_rec_len;
		
		uart_rec_data[uart_rec_len] = '\0';
		memcpy((INT8U *)net_send_data, (INT8U *)uart_rec_data, uart_rec_len);

		//clear uart and begin to recv new 
		uart_rec_len = 0;
		net_sendStart = 1;
	}
}

/*
 * process mode switch function. when uart receive one frame only obtain "+++" string,
 * then switch to command mode.
 **/
VOID UartRecvThread(VOID *arg)
{
	void *pMesg;
	UINT8 err;
	NST_InitTimer(&pTimer, timerFuncProcess, NULL, NST_TRUE);

	uartMessgSem = OSMboxCreate(NULL);
	while (1) {
		pMesg = OSMboxPend(uartMessgSem, 0, &err);
		
		if(strcmp((char *)pMesg,"start timer") == 0)	
		NST_SetTimer(pTimer, UserParam.frameGap);

		if(strcmp((char *)pMesg,"cancelled timer") == 0)
		NST_CancelTimer(pTimer, &isCancelled);
		
		if (pMesg != NULL && strcmp((char *)pMesg,"-14") == 0) {
			printf("+ERROR=%s\n\r", (char *)pMesg);
			OSTimeDly(50);
		}
	}
}

VOID SoftApConfThread(VOID *arg)
{
	int ret;
	ip_addr_t ipAddr;
	int i,j,ssid_len,password_len;
	int bytes_read;
	struct timeval timeout;
	INT32 sockfd = 0;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	UINT16 socketPort = 60001;
	char wifissid[33];
	char ssid[33];
	char password[64];

	u32_t addr_len = sizeof(struct sockaddr);
	char *recv_data;
	recv_data = OSMMalloc(g_RecvBufSize);

	if (recv_data == NULL)
	{
       DBGPRINT(DEBUG_ERROR, "+ERROR=No memory\r\n");
       goto END;
	}

	timeout.tv_sec = 3;
	timeout.tv_usec = 0;

	InfWiFiStop();
	OSTimeDly(10);

	InfNetModeSet(PARAM_NET_MODE_SOFTAP);
	inet_aton("10.10.10.1",&ipAddr);
	SysParam.IPCfg.Ip[0] = ip4_addr1(&ipAddr);
	SysParam.IPCfg.Ip[1] = ip4_addr2(&ipAddr);
	SysParam.IPCfg.Ip[2] = ip4_addr3(&ipAddr);
	SysParam.IPCfg.Ip[3] = ip4_addr4(&ipAddr);
	SysParam.WiFiCfg.Encry = 3;
	SysParam.WiFiCfg.AuthMode = 3;
	sprintf((char *)wifissid, "NL6621-%02x%02x%02x%02x", PermanentAddress[2], PermanentAddress[3], PermanentAddress[4], PermanentAddress[5]); 
	InfSsidSet((INT8U *)wifissid, strlen((char *)wifissid));
	InfKeySet(0, "12345678", 8);

	InfWiFiStart();
	OSTimeDly(20);

	if(link_status != SYS_EVT_LINK_UP){
		DBGPRINT(DEBUG_ERROR, "+ERROR=WiFi Started Failed\r\n");
		goto END;
	}else{
		if((sockfd = socket(AF_INET, SOCK_DGRAM,0)) == -1){
			printf("+ERROR=%d\r\n",INVALID_UDP_SOCKET);
			goto END;
		}

		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(socketPort);
		server_addr.sin_addr.s_addr = INADDR_ANY;
		memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

		ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		if(ret == -1){
			DBGPRINT(DEBUG_ERROR, "+ERROR = %d\r\n", INVALID_TCP_SETOPT);
		}

		if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1){
			DBGPRINT(DEBUG_ERROR, "+ERROR = %d\r\n", INVALID_BCTRX_BIND);
			goto END;
		}
		
		DBGPRINT(DEBUG_TRACE, "+OK:SoftApConfig...\r\n");

		while(1){
			if(link_status != SYS_EVT_LINK_UP){
				DBGPRINT(DEBUG_ERROR, "+ERROR=WiFi isn't started\r\n");
				goto END;
			}

			bytes_read = recvfrom(sockfd, recv_data, g_RecvBufSize, 0, 
				(struct sockaddr *)&client_addr, &addr_len);
				
			if(bytes_read <= 0 || bytes_read != 200){
				continue;
			}

			for(i = 0; i < 8; i++)
			{
				if(recv_data[i] != i)
				{
					break;
				}
			}

			if(i != 8){
				continue;
			}

			ssid_len = recv_data[i];
			if(ssid_len > 32 || ssid_len == 0){
				DBGPRINT(DEBUG_ERROR, "+ERROR=ssid length invalid\r\n");
				goto END;
			}

			i++;

			for(j = 0; j < ssid_len; j++)
			{
				ssid[j] = recv_data[i+j];
			}
			ssid[j+1] = '\0';

			password_len = recv_data[i+ssid_len];
			if(password_len > 64){
				DBGPRINT(DEBUG_ERROR, "password length invaild\r\n");
				goto END;
			}

			for(j = 0; j < password_len; j++)
			{
				password[j] = recv_data[i+ssid_len+1+j];
			}
			password[j+1] = '\0';

			InfWiFiStop();
			OSTimeDly(10);
			InfNetModeSet(PARAM_NET_MODE_STA_BSS);
			InfSsidSet((UINT8 *)ssid, strlen(ssid));    /* set ssid */
			InfKeySet(0, (UINT8 *)password, strlen(password));
			InfConTryTimesSet(5);
			SysParam.WiFiCfg.Encry = 4;
			InfWiFiStart();
			OSTimeDly(30);
			goto END;
		}
	}
			
END:
	if(recv_data != NULL){
		OSMFree(recv_data);
	}

	if(sockfd >= 0){
		lwip_close(sockfd);
	}
	memset(&ssid, 0, strlen(ssid));
	memset(&password, 0, strlen(password));
	OSTaskDel(OS_PRIO_SELF);
}
#ifdef TEST_AIRKISS
int TestAirkissFlag = 0;
airkiss_context_t AirkissCtx;
airkiss_config_t AirkissCfg;
airkiss_result_t AirkissRes;
VOID TestAirkissRecvCb(UINT8 *pRecvBuf, UINT32 pRecvLen)
{
    UINT32 Ret = 0;

    Ret = airkiss_recv(&AirkissCtx, pRecvBuf, pRecvLen);
    if (Ret == AIRKISS_STATUS_CHANNEL_LOCKED)
        TestAirkissFlag = 2;
    else if (Ret == AIRKISS_STATUS_COMPLETE)
        TestAirkissFlag = 3;
}

VOID BroadcastRand()
{
    int ret = 0;
    int count = 0;
    
    int udp_sock;
    const int opt = 1;    
    struct sockaddr_in addrto;
    int nlen = sizeof(addrto);
    
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    {
        DBGPRINT(DEBUG_ERROR, "UDP Broadcast Socket error\n");
        return;
    }
    
    if (setsockopt(udp_sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt)) == -1) 
    {  
        DBGPRINT(DEBUG_ERROR, "set UDP Broadcast socket error...\n");  
		lwip_close(udp_sock);  
        return;  
    }  
    
    memset(&addrto, 0, sizeof(struct sockaddr_in));  
    addrto.sin_family = AF_INET;  
    addrto.sin_addr.s_addr = htonl(INADDR_BROADCAST);  
    addrto.sin_port=htons(10000);  
    
    DBGPRINT(DEBUG_TRACE, "Respond with udp broadcast.");
    
    while (1)
    {
        ret = sendto(udp_sock, &AirkissRes.random, 1, 0, (struct sockaddr *)&addrto, nlen);
        if (ret < 0) 
        {
            continue;
        } 
        
        /* print debug message to console */
        if (count < 5 * 10)  // broadcast 5s
        {
            if (count % 200 == 0) 
                printf("\b..");
    
            if (count % 3 == 0) 
                printf("\b\\");
            else if (count % 3 == 1) 
                printf("\b-");
            else if (count % 3 == 2) 
                printf("\b/");
                
            OSTimeDly(10);
        } 
        else
        {
            printf("\n");
            break;
        }
        count++;
    }

    lwip_close(udp_sock);
    DBGPRINT(DEBUG_TRACE, "Respond finished.\n");
}



//AIRKISS配置
VOID AirKissConfThread(VOID *arg)
{
    SCAN_INFO_TABLE * pScanTable = NULL;
    UINT8 i, j;
    
    InfWiFiStop();
    OSTimeDly(5);
    // init airkiss
    AirkissCfg.memset = memset;
    AirkissCfg.memcpy = memcpy;
    AirkissCfg.memcmp = memcmp;
    //AirkissCfg.printf = printf; // not necessary
    airkiss_init(&AirkissCtx, &AirkissCfg);

    // scan all channel to get ap list
    pScanTable = OSMMalloc(sizeof(SCAN_INFO_TABLE) + sizeof(SCAN_INFO) * (32 -1));
    if (!pScanTable)
    {
        DBGPRINT(DEBUG_TRACE, "AirKiss: Test airkiss out of memory\r\n");
        return;
    }
    DBGPRINT(DEBUG_TRACE, "+OK:AirKissConfig...\r\n");   
SCAN_LOOP:
    TestAirkissFlag = 0;
    InfSsidSet(0, 0);
    InfNetModeSet(PARAM_NET_MODE_STA_BSS);
    InfWiFiStart();
    pScanTable->InfoCount = 32;   
    InfScanStart(pScanTable, 0, 0);

    // wait scan done
    while (TestAirkissFlag != 1)
        OSTimeDly(10);
    InfWiFiStop();
    
    // sniff packets of every ap in the scan info list
    for (i = 0; i < pScanTable->InfoCount; i++)
    {
        InfSnifferStart(TestAirkissRecvCb, pScanTable->InfoList[i].Bssid, pScanTable->InfoList[i].Channel);
        for (j = 0; j < 50; j++)
        {
            // wait channel lock
            if (TestAirkissFlag >= 2)
            {
                break;
            }
            OSTimeDly(2);
        }

        if (TestAirkissFlag >= 2)
            break;
        InfWiFiStop();
    }

    if (TestAirkissFlag >= 2)
    {
        for (j = 0; j < 6000; j++)// wait 60s
        {
            if (TestAirkissFlag >= 3)
            {
                break;
            }
            OSTimeDly(2);
        }
    }
    else
        goto SCAN_LOOP;

    InfWiFiStop();
    if (TestAirkissFlag >= 3)
    {
        DBGPRINT(DEBUG_TRACE, "AirKiss ok\r\n");
        TestAirkissFlag = 0;
        airkiss_get_result(&AirkissCtx, &AirkissRes);
        InfSsidSet((UINT8 *)AirkissRes.ssid, AirkissRes.ssid_length);
        InfKeySet(0, (UINT8 *)AirkissRes.pwd, AirkissRes.pwd_length);
		InfConTryTimesSet(5);
        InfEncModeSet(PARAM_ENC_MODE_AUTO);
        InfWiFiStart();
		OSTimeDly(5);
        /* wait wifi link up */
        while (TestAirkissFlag == 0)
            OSTimeDly(30);
        BroadcastRand();
	    
    }
    else
        DBGPRINT(DEBUG_TRACE, "AirKiss timeout\r\n");

    OSMFree(pScanTable);
    DBGPRINT(DEBUG_TRACE, "Test AirKiss done\r\n");
	OSTaskDel(OS_PRIO_SELF);
	
}

#endif

void uart_data_recv(char Dummy)
{
    static unsigned char recv_start = 0;

	/* when in data mode ,'+++' means escape */
	if (UserParam.atMode == DATA_MODE) {
			
		if(uart_rec_len < MAX_RECV_BUFFER_SIZE){
			uart_rec_data[uart_rec_len] = Dummy;
			OSMboxPost(uartMessgSem ,(void *)"start timer");
			uart_rec_len++;
		}

		/* If these is no client connect to server or connect to server failed,
		 * only	catch "+++" string and filter the other char.
		 * */
		if (WifiConnStatus.connStatus != CONNECT_STATUS_OK) {
			if ((uart_rec_len == 3)) {
			uart_rec_data[3] = '\0';
				if (strcmp(uart_rec_data, "+++") == 0) {
					OSSemPost(modeSwitchSem);
					OSMboxPost(uartMessgSem ,(void *)"cancelled timer");
				}else{
					uart_rec_len = 0;
				}	
			} else if (uart_rec_len > 5) {
				uart_rec_len = 0;
			}
			return;		
		}
		
		if(uart_rec_len == MAX_RECV_BUFFER_SIZE){
			if( end_sendFlags < 3){
				if(strcmp(&Dummy,"+") == 0){
					end_sendFlags++;
				}else{
					end_sendFlags = 0;
					if (uartMessgSem != NULL)
					OSMboxPost(uartMessgSem ,(void *)"-14");
				}
			}else if(end_sendFlags == 3){ 
				if(strcmp(&Dummy,"\r") == 0)
					end_sendFlags++;
				else
					end_sendFlags = 0;
			}else if(end_sendFlags == 4){
				if(strcmp(&Dummy,"\n") == 0){
					memset(uart_send_data, '\0', MAX_RECV_BUFFER_SIZE);
					end_sendFlags = 0;
					uart_rec_len = 3;
					strcpy(uart_rec_data,"+++");
					uart_rec_data[3] = '\0';
					OSSemPost(modeSwitchSem);
					OSMboxPost(uartMessgSem ,(void *)"cancelled timer");
				}else
					end_sendFlags = 0;
			}
		}
		
		if ((uart_rec_len == 3)) {
			uart_rec_data[3] = '\0';
			if (strcmp(uart_rec_data, "+++") == 0) {
				OSSemPost(modeSwitchSem);
				OSMboxPost(uartMessgSem ,(void *)"cancelled timer");
			}

		}else if (uart_rec_len % (UserParam.frameLength) == 0 && uart_rec_len < MAX_RECV_BUFFER_SIZE) {
			/* make sure module in connect status */
			if ((WifiConnStatus.connStatus == CONNECT_STATUS_OK) 
				&& ((link_status == SYS_EVT_LINK_UP) 
				|| (link_status == SYS_EVT_SCAN_DONE))) {
				if (net_sendStart == 0)	{
					uart_rec_data[uart_rec_len] = '\0';
					net_send_len = uart_rec_len;
					uart_rec_len = 0;
					memcpy((INT8U *)net_send_data, (INT8U *)uart_rec_data, net_send_len);
					net_sendStart = 1;
				} /*else {
						if (uartMessgSem != NULL)
						OSMboxPost(uartMessgSem ,(void *)"-14");
				}*/
			} else {
				if (uartMessgSem != NULL)
					OSMboxPost(uartMessgSem ,(void *)"-18");
			}
		}
	} else {
       if(uart_recvEnd == 0){
			if(Dummy == 0x0A ) {	/* 'enter' keybad, carriage return) */ 
			    if((uart_rec_data[0] == 'A' || uart_rec_data[0] == 'a') && (uart_rec_data[1] == 'T' || uart_rec_data[1] == 't')){
					 if(uart_rec_data[2] == '+' || uart_rec_data[2] == 0x0D){
					    uart_rec_len--;//不包括0x0A
						uart_recvEnd = 1;
				        recv_start = 0;	
			            uart_rec_data[uart_rec_len] = '\0';	 
					 } else {
					 	uart_rec_len = 0;
		                recv_start = 0;	 
					 }
			   }
			} else {
				uart_rec_data[uart_rec_len] = Dummy;

			    if(recv_start == 0){
					if(uart_rec_data[uart_rec_len] == 'A' || uart_rec_data[uart_rec_len] == 'a'){
					   uart_rec_data[0] = 'A';
					   uart_rec_len = 0;
					}
	
					if(uart_rec_len == 1 && (uart_rec_data[1] == 'T' || uart_rec_data[1] == 't') && uart_rec_data[0] == 'A' || uart_rec_data[0] == 'a'){
				       recv_start = 1;
					} else {
					   uart_rec_len = 0;  
					}
			   }
			   if(uart_rec_len++ > 200)	//AT指令不可能超过200.
			   {
			   	  recv_start = 0;
				  uart_rec_len = 0;
				  uart_recvEnd = 0;
			   }
			}
	   }
	}	
}

/***********************************************************/
/**************** External called interface ****************/
/***********************************************************/


