

#include "socketCom.h"
#include "ring_buffer.h"
#include "sockets.h"

extern SYS_EVT_ID link_status;
extern ring_buffer_t uartrxbuf;
extern UINT8 	net_sendStart;
extern UINT8 uart_send_flag;
extern NST_LOCK * recv_lock;	/* receive lock */

/* wifi connect status struct */
CONN_STATUS WifiConnStatus;

char send_buf[g_SendBufSize];

lanserver ls;

void UDPSendTask(unsigned char type)
{	
	int sendcount;
	struct sockaddr_in local_addr;
	char *pdata;
	int Err;

	UINT32	data_len = 0;

//	while  {
//		OSTimeDly(100);	
//	}

	while (1) {
		if (UserParam.atMode == DATA_MODE) {
				if(WifiConnStatus.connStatus == CONNECT_STATUS_FAIL){
					OSTimeDly(20);		
					continue;		
				}

			
				if (type == SOCK_CLIENT) {
					memcpy(&local_addr, &WifiConnStatus.server_addr, sizeof(struct sockaddr_in));
							
				} else if (type == SOCK_SERVER) {
					memcpy(&local_addr, &WifiConnStatus.client_addr, sizeof(struct sockaddr_in));
				}
				
				OSSemPend(sendSwitchSem, 0, &Err);
				data_len = ring_buf_cnt(&uartrxbuf);
				
				if(data_len == 0){
					continue;
				}else if(data_len <= g_SendBufSize){
	
					data_len = ring_buf_read(&uartrxbuf, send_buf,data_len);						
				}else{
					data_len = ring_buf_read(&uartrxbuf, send_buf,g_SendBufSize);
				}

				net_sendStart = SEND_DONE;
							
				sendcount = sendto(WifiConnStatus.sockfd, send_buf, data_len, 0, (struct sockaddr *)&local_addr, sizeof(struct sockaddr));
				if (sendcount < 0) {
					continue;							
				}
			}else{
				
				return;
			}
	}
}

void UDPRecvTask(unsigned char type)
{
	int ret;
	int bytes_read;
	int count = 0;
	int i = 0;
	int timeout = 3;	
    
	u32_t addr_len = sizeof(struct sockaddr);
	char *recv_data;
	recv_data = OSMMalloc(g_RecvBufSize);

	/* create socket : type is SOCK_DGRAM, UDP */
	if ((WifiConnStatus.sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	   printf("+ERROR= %d\n\r", INVALID_UDP_SOCKET);
	   goto END;
	}

	if (type == SOCK_CLIENT) {
	    WifiConnStatus.server_addr.sin_family = AF_INET;
	    WifiConnStatus.server_addr.sin_port = htons(WifiConnStatus.socketPort);
	    WifiConnStatus.server_addr.sin_addr.s_addr = WifiConnStatus.remoteIp;
	    memset(&(WifiConnStatus.server_addr.sin_zero), 0, sizeof(WifiConnStatus.server_addr.sin_zero));

		ret = setsockopt(WifiConnStatus.sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		if (ret == -1) {
		   	printf("+ERROR= %d\n\r", INVALID_TCP_SETOPT);
		}

		/*
		 * After socket create, set connect success, resume send task thread.
		 **/
		WifiConnStatus.connStatus = CONNECT_STATUS_OK;
					
	} else if (type == SOCK_SERVER) {
		WifiConnStatus.server_addr.sin_family = AF_INET;
		WifiConnStatus.server_addr.sin_port = htons(WifiConnStatus.socketPort);
		WifiConnStatus.server_addr.sin_addr.s_addr = INADDR_ANY;
		memset(&(WifiConnStatus.server_addr.sin_zero), 0, sizeof(WifiConnStatus.server_addr.sin_zero));	

		/* bing server */
		if (bind(WifiConnStatus.sockfd, (struct sockaddr *)&WifiConnStatus.server_addr,
			   sizeof(struct sockaddr)) == -1) {
			printf("+ERROR= %d\n\r", INVALID_BIND);
			goto END;
		}
		
		ret = setsockopt(WifiConnStatus.sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		if (ret == -1) {
		   	printf("+ERROR= %d\n\r", INVALID_TCP_SETOPT);
		}	
	}

	printf("UDP %s Waiting/Connecting on port %d...\n", 
			((type == SOCK_CLIENT) ? "client" : "server"), WifiConnStatus.socketPort);

	while (1) {
		if (UserParam.atMode == DATA_MODE) {

			if (WifiConnStatus.sockfd != -1) {

				bytes_read = recvfrom(WifiConnStatus.sockfd, recv_data, g_RecvBufSize, 0,
			                         (struct sockaddr *)&WifiConnStatus.client_addr, &addr_len);


				if (bytes_read <= 0) {
					continue;
				}
				
				/* when client connect local server first, save the client connect attributation
				 * and launch send task, else filter it 
				 **/
				if ((type == SOCK_SERVER) && (count <= 0)) {
					WifiConnStatus.connStatus = CONNECT_STATUS_OK;
					count++;
				}
	
				i = 0;
				while(i < bytes_read)
				{
					BSP_UartPutcPolled(*(recv_data+i));
					i++;	
				}

				memset(recv_data, 0, g_RecvBufSize);
				//OSTimeDly(10);
				} 
		}else if(UserParam.atMode == AT_MODE){
				goto END;
			}
	}
	

END:
	WifiConnStatus.connStatus = CONNECT_STATUS_FAIL;

	if(WifiConnStatus.sockfd >= 0)
	{
		lwip_close(WifiConnStatus.sockfd);
	 	WifiConnStatus.sockfd = -1;
	}

	if (recv_data != NULL) 
		OSMFree(recv_data);
}

#define TCP_ONEFRAME_SEND_SWITCH		(0)

void TCPSendTask(unsigned char type)
{
	int snd, i;
	int socketFd = -1;
	UINT8  FailCnt = 0;
	UINT32	data_len = 0;
	int Err;


#if TCP_ONEFRAME_SEND_SWITCH
	char *pdata;
	int sendcount;
	UINT32	data_len = 0;
	UINT32	send_fail_cnt = 0;
#endif

		
	while (1) {
		if (UserParam.atMode == DATA_MODE) {

			if(WifiConnStatus.sockfd < 0 && Get_MaxFd() <= 0){
				OSTimeDly(20);		
				continue;		
			}

			OSSemPend(sendSwitchSem, 0, &Err);

			NST_AQUIRE_LOCK(recv_lock);
			data_len = ring_buf_cnt(&uartrxbuf);
			
			if(data_len == 0){
				continue;
			}else if(data_len <= g_SendBufSize){

				data_len = ring_buf_read(&uartrxbuf, send_buf,data_len);						
			}else{
				data_len = ring_buf_read(&uartrxbuf, send_buf,g_SendBufSize);
			}
			NST_RELEASE_LOCK(recv_lock);
			
			net_sendStart = SEND_DONE;

			if(type == SOCK_SERVER)
			{
				for(i = 0;i < LAN_TCPCLIENT_MAX; i++)
				{
					if(ls.tcpClient[i].fd >= 0)
					{		
						snd = send(ls.tcpClient[i].fd, send_buf, data_len, 0);
						
						if (snd < 0) {
							while(1)
							{
								if(UserParam.atMode == AT_MODE || Get_MaxFd() <= 0)
								{
									ring_buf_clear(&uartrxbuf);
										
									if(uart_send_flag == UART_STOP)
									{
										uart_send_flag = UART_START;
										BSP_GPIOSetValue(UART_RTS, GPIO_HIGH_LEVEL);
									}

									return;
								}

								OSTimeDly(5);
								snd = send(ls.tcpClient[i].fd, send_buf, data_len, 0);

								if(snd > 0){
									if(uart_send_flag == UART_STOP)
									{
										uart_send_flag = UART_START;
										BSP_GPIOSetValue(UART_RTS, GPIO_HIGH_LEVEL);
									}
									break;
								}

							}
					    } else {
							if(uart_send_flag == UART_STOP)
							{
								uart_send_flag = UART_START;
								BSP_GPIOSetValue(UART_RTS, GPIO_HIGH_LEVEL);
							}
						}
					}
				}

			}else if(type == SOCK_CLIENT)
			{
				snd = send(WifiConnStatus.sockfd, send_buf, data_len, 0);
				if (snd < 0) {
					while(1)
					{
						if(UserParam.atMode == AT_MODE || WifiConnStatus.sockfd < 0)
						{
							ring_buf_clear(&uartrxbuf);
								
							if(uart_send_flag == UART_STOP)
							{
								uart_send_flag = UART_START;
								BSP_GPIOSetValue(UART_RTS, GPIO_HIGH_LEVEL);
							}

							return;
						}

						OSTimeDly(5);
						snd = send(WifiConnStatus.sockfd, send_buf, data_len, 0);

						if(snd > 0){
							if(uart_send_flag == UART_STOP)
							{
								uart_send_flag = UART_START;
								BSP_GPIOSetValue(UART_RTS, GPIO_HIGH_LEVEL);
							}
							break;
						}

					}
			    } else {
					if(uart_send_flag == UART_STOP)
					{
						uart_send_flag = UART_START;
						BSP_GPIOSetValue(UART_RTS, GPIO_HIGH_LEVEL);
					}
				}	
			}
												
		} else {
	  		return;
	  	}
	
						
	}
		
}

/****************************************************************
        FunctionName        :   LAN_tcpClientInit.
        Description         :      init tcp clients.
        Add by Vshawn     --2015-11-24
****************************************************************/
int32 LAN_tcpClientInit(void)
{
    int32 i;

    for (i = 0; i < LAN_TCPCLIENT_MAX; i++)
    {
        memset(&(ls.tcpClient[i]), 0x0, sizeof(ls.tcpClient[i]));
        ls.tcpClient[i].fd = -1;
        ls.tcpClient[i].isLogin = LAN_CLIENT_LOGIN_FAIL;
        ls.tcpClient[i].timeout = 0;
    }

    return  0;
}

void AddSelectFD(void)
{
	int32 i = 0;
	FD_ZERO( &(ls.readfd) );
	   
	if(ls.tcpServerFd >= 0 )
	{
    	FD_SET(ls.tcpServerFd, &(ls.readfd) );
	}

	for(i = 0; i < LAN_TCPCLIENT_MAX; i++)
    {
        if( ls.tcpClient[i].fd >= 0 )
        {
            FD_SET( ls.tcpClient[i].fd, &(ls.readfd) );
        }
    }
}

int32 Get_MaxFd(void)
{
	int i;
	int32 maxfd = 0;
	
	if( maxfd <= ls.tcpServerFd )
    maxfd = ls.tcpServerFd;

	for(i = 0; i < LAN_TCPCLIENT_MAX; i++)
    {
        if( ls.tcpClient[i].fd >= 0 )
        {
            maxfd = ls.tcpClient[i].fd;
        }
    }

	return maxfd;
}

int32 tcpClient_select(int32 nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds,int32 sec,int32 usec )
{
    struct timeval t;
    
    t.tv_sec = sec;// 秒
    t.tv_usec = usec;// 微秒
    return select( nfds,readfds,writefds,exceptfds,&t );
}

int tcpClient_SelectFd(int32 sec,int32 usec)
{
	int32 ret = 0;
	int32 select_fd = 0;
	AddSelectFD();
	select_fd = Get_MaxFd();

	if( select_fd >= 0 )
    {
        ret = tcpClient_select(select_fd+1,&(ls.readfd),NULL,NULL,sec,usec );
        if( ret==0 )
        { 
            //Time out.
        }
    }
    return ret;
}

int32 CreateTcpServer( uint16 tcp_port )
{
    struct sockaddr_t addr;
    int32 bufferSize=0;
    int32 serversocketid=0;
	int tmp = 1;
	struct timeval timeout;

	int keepAlive = 1; // 开启keepalive属性
	int keepIdle = 30; // 如该连接在30秒内没有任何数据往来,则进行探测 
	int keepInterval = 5; // 探测时发包的时间间隔为5 秒
	int keepCount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.

	timeout.tv_sec = 3;   /* receive data timeout: 3 seconds */
    timeout.tv_usec = 0; 

    serversocketid = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if( serversocketid < 0 )
    {
        serversocketid = INVALID_SOCKET;
        printf("TCPServer socket create error\n\r");
        return 1;
    }
    bufferSize = SOCKET_TCPSOCKET_BUFFERSIZE;
//    setsockopt( serversocketid, SOL_SOCKET, SO_RCVBUF, &bufferSize, 4 );
//    setsockopt( serversocketid, SOL_SOCKET, SO_SNDBUF, &bufferSize, 4 );
//	setsockopt( serversocketid, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	setsockopt( serversocketid, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
	setsockopt( serversocketid, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
	setsockopt( serversocketid, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
	setsockopt( serversocketid, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));
	setsockopt( serversocketid, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));

    memset(&addr, 0x0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(tcp_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if( bind( serversocketid, (struct sockaddr *)&addr, sizeof(addr)) != 0 )
    {
        printf("TCPSrever socket bind error\n\r");
        close(serversocketid);
        serversocketid = INVALID_SOCKET;
        return 1;
    }

    if(listen( serversocketid, LAN_TCPCLIENT_MAX ) != 0 )
    {
        printf("TCPServer socket listen error!\n\r");
        close( serversocketid );
        serversocketid = INVALID_SOCKET;
        return 1;
    }
	printf("TCP Server socketid:%d on port:%d\n\r", serversocketid, tcp_port);
    return serversocketid;

}

int32 AddTcpNewClient(int fd, struct sockaddr_t *addr)
{
    int32 i;
    
    if(fd < 0)
    {
        return 1;
    }

    for(i = 0; i < LAN_TCPCLIENT_MAX; i++)
    {
        if(ls.tcpClient[i].fd == INVALID_SOCKET)
        {
            ls.tcpClient[i].fd = fd;
            //Lan_setClientTimeOut(pgc, i);

            return 0;
        }
    }

    printf("[LAN]tcp client over %d channel, denied!", LAN_TCPCLIENT_MAX);
    close(fd);
    
    return 1;
}

void TCPRecvTask(unsigned char type)
{
	int socketFd;
	fd_set fdR;
	int select_ret;

	int ret, i = 0, j = 0;
	int tmp = 1;
	int bytes_read;
	char *recv_data;
	struct timeval timeout;
	
	u32_t sin_size = sizeof(struct sockaddr_in);
	recv_data = OSMMalloc(g_RecvBufSize);
	memset(recv_data, 0, g_RecvBufSize);

	timeout.tv_sec = 3;   /* receive data timeout: 3 seconds */
    timeout.tv_usec = 0; 


	/* Create socket and connect to server */
	while (1) {	
		if (UserParam.atMode == DATA_MODE) {
				
			if (type == SOCK_CLIENT) {
				int	nfds;

				/* create socket:type is SOCK_STREAM, TCP Client */
				if ((WifiConnStatus.sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
				   printf("+ERROR= %d\n\r", INVALID_TCP_SOCKET);
				   goto END;
				}

			    memset(&(WifiConnStatus.server_addr), 0, sizeof(WifiConnStatus.server_addr));
			    WifiConnStatus.server_addr.sin_family = AF_INET;
			    WifiConnStatus.server_addr.sin_port = htons(WifiConnStatus.socketPort);
			    WifiConnStatus.server_addr.sin_addr.s_addr = WifiConnStatus.remoteIp;
			    memset(&(WifiConnStatus.server_addr.sin_zero), 0, sizeof(WifiConnStatus.server_addr.sin_zero));

				setsockopt(WifiConnStatus.sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

				printf("TCP client connecting on port %d...\n", WifiConnStatus.socketPort);
					
				if (connect(WifiConnStatus.sockfd, (struct sockaddr *)&WifiConnStatus.server_addr,
				        sizeof(struct sockaddr)) == -1)	{
					printf("+ERROR= %d\n\r", INVALID_TCP_CONNECT);
				   	OSTimeDly(100);
					goto END;
				} else {

				    printf("+OK:Connected Server\n");
					WifiConnStatus.connStatus = CONNECT_STATUS_OK;

					while (1) {
						if(UserParam.atMode == AT_MODE ||
							 WifiConnStatus.connStatus == CONNECT_STATUS_FAIL)
						{
							 goto END;
						}
		
						bytes_read = recv(WifiConnStatus.sockfd, recv_data, g_RecvBufSize, 0);					
						if(bytes_read > 0)
						{
							j = 0;
							while(j < bytes_read)
							{
								BSP_UartPutcPolled(*(recv_data+j));
								j++;	
							}

							memset(recv_data, 0, g_RecvBufSize);
						}else if(bytes_read == 0){
							goto END;
						}
					}
				}
										
			} else if (type == SOCK_SERVER) {
				int newfd = -1,ret = -1,fd = -1;
				struct sockaddr_t addr;
				int addrLen = sizeof(struct sockaddr_t);

				LAN_tcpClientInit();

				
				ls.tcpServerFd = CreateTcpServer(WifiConnStatus.socketPort);

				while(1)
				{
					if(UserParam.atMode == AT_MODE)
					{
					 	goto END;
					}

					tcpClient_SelectFd(0, 0);

					if(ls.tcpServerFd < 0)
					{
						printf("tcp server socket err\n\r");
						goto END;
					}
					
					if(FD_ISSET(ls.tcpServerFd, &(ls.readfd)))
					{
						newfd = accept(ls.tcpServerFd, &addr, (socklen_t *)&addrLen);
						WifiConnStatus.connectFd = newfd;
						if(newfd < 0)
						{
							printf("Neeed to Restart Lan_TcpServer\n\r");
							goto END;
						}
						printf("detected new client as %d\n\r", newfd);
						WifiConnStatus.connStatus = CONNECT_STATUS_OK;
						ret = AddTcpNewClient(newfd, &addr); 
					}

					for(i = 0;i < LAN_TCPCLIENT_MAX; i++)
					{
						fd = ls.tcpClient[i].fd;
						if(fd < 0)
						continue;

						if(FD_ISSET(fd, &(ls.readfd)))
						{
							ret = recv(fd, recv_data, g_RecvBufSize, 0);
							if(ret > 0)
							{
								j = 0;
								while(j < ret)
								{
									BSP_UartPutcPolled(*(recv_data+j));
									j++;	
								}

								memset(recv_data, 0, g_RecvBufSize);
							}else if(ret <= 0){
								close(ls.tcpClient[i].fd);
								ls.tcpClient[i].fd = -1;
							}
						}
					}
					
				}												
			}					
		}/*end if UserParam.atMode == DATA_MODE*/
		 else {
			goto END;
		}
	}

END:
	WifiConnStatus.connStatus = CONNECT_STATUS_FAIL;
	
	if(type == SOCK_SERVER)
	{
		for(i = 0;i < LAN_TCPCLIENT_MAX; i++)
		{
			if(ls.tcpClient[i].fd >= 0)
			{
				close(ls.tcpClient[i].fd);
				ls.tcpClient[i].fd = -1;
			}	
		}

		if(ls.tcpServerFd >= 0)
		{
			close(ls.tcpServerFd);
			ls.tcpServerFd = -1;
		}
	}

	if(WifiConnStatus.sockfd >= 0)
	{
		lwip_close(WifiConnStatus.sockfd);
		WifiConnStatus.sockfd = -1;
	}
	
	if (recv_data != NULL) 
		OSMFree(recv_data);
}


VOID SendThread(VOID *arg)
{
	while (1) {
		/* waiting for device link up */
		while (link_status != SYS_EVT_LINK_UP) {
			OSTimeDly(200);
		}

		if (UserParam.atMode == DATA_MODE) {
			switch (UserParam.socketProtocol) {
				case 0:	/* UDP connect */
					if (UserParam.socketType == SOCK_CLIENT) {
						UDPSendTask(SOCK_CLIENT);
					} else if (UserParam.socketType == SOCK_SERVER) {
						UDPSendTask(SOCK_SERVER);
					} 
					break;

				case 1:	/* TCP connect */
					if (UserParam.socketType == SOCK_CLIENT) {
						TCPSendTask(SOCK_CLIENT);
					} else if (UserParam.socketType == SOCK_SERVER) {
						TCPSendTask(SOCK_SERVER);
					}
					break;
			}
		
		} else {
			OSTimeDly(20);	
		}
	}
}



VOID RecvThread(VOID *arg)
{
	while (1) {
		/* waiting for device link up */
		while (link_status != SYS_EVT_LINK_UP) {
			OSTimeDly(200);
		}

		if (UserParam.atMode == DATA_MODE) {
			switch (UserParam.socketProtocol) {
				case 0:	/* UDP connect */
					if (UserParam.socketType == SOCK_CLIENT) {
						UDPRecvTask(SOCK_CLIENT);	
					
					} else if (UserParam.socketType == SOCK_SERVER) {
					   	UDPRecvTask(SOCK_SERVER);

					}
					break;

				case 1:	/* TCP connect */
					if (UserParam.socketType == SOCK_CLIENT) {
						TCPRecvTask(SOCK_CLIENT);					
					} else if (UserParam.socketType == SOCK_SERVER) {
						TCPRecvTask(SOCK_SERVER);
					}
					break;

			}
		
		} else {
			OSTimeDly(20);	
		}
	}
}

/*
******************************************************************************
**                        void RespondUdpBroadcast(void)
**
** Description  : respond with udp broadcast packet for 1.5 seconds.
** Arguments    : 
                  
** Returns      : 
** Author       :                                   
** Date         : 
**
******************************************************************************
*/
VOID RespondUdpBroadcast(VOID)
{    
    int udp_sock;
    const int opt = 1;    
    struct sockaddr_in addrto;
    int nlen = sizeof(addrto);
    char sendbuf[24] = {0}; 

    /* fill with response data */
    sendbuf[0] = 0x18;
    sendbuf[8] = 0x06;
    sendbuf[10] = 0x01;
    
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {        
        return;
    }
    
    if (setsockopt(udp_sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt)) == -1) {
		lwip_close(udp_sock);    
        return;  
    }  
    
    memset(&addrto, 0, sizeof(struct sockaddr_in));  
    addrto.sin_family = AF_INET;  
    addrto.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    addrto.sin_port=htons(60002);

	sendto(udp_sock, sendbuf, 24, 0, (struct sockaddr *)&addrto, nlen);
	lwip_close(udp_sock);
}


#define UDP_BRX_DATA_LEN_MAX 			(256)

/*
 * UDP broadcast task thread.
 **/
VOID BCTRecvThread(VOID * arg)
{
	int ret;
    fd_set fdset;
	struct timeval timeout;

	int recv_count;
    socklen_t addr_len = sizeof(struct sockaddr);
	char buffer[UDP_BRX_DATA_LEN_MAX + 1];

	timeout.tv_sec = 0;   /* receive data timeout: 1 seconds */
    timeout.tv_usec = 500 * 1000;

    while (1) {
        if ((broadcast_status.rx_status == 1) && 
			(link_status == SYS_EVT_LINK_UP)) {
			
			if (broadcast_status.rx_sockfd > 0) {
				//printf("+ERROR=%d\n\r", INVALID_BCTRX_SOCK);
				OSTimeDly(100);
				continue;	
			}		

			FD_ZERO(&fdset); 
        	FD_SET(broadcast_status.rx_sockfd, &fdset);
			
			ret = select(broadcast_status.rx_sockfd + 1, &fdset, NULL, NULL, &timeout);
	        if (ret < 0) {
	            printf("+ERROR=%d\n\r", INVALID_BCTRX_SELECT);
	            continue;
	        } else if (ret == 0) {
				//printf("Select timeout.\n\r");
				continue;
			}

            recv_count = recvfrom(broadcast_status.rx_sockfd, buffer, UDP_BRX_DATA_LEN_MAX, 0,
                (struct sockaddr *)&broadcast_status.rx_client_addr, &addr_len);
			if (recv_count > 0) {
				buffer[recv_count] = '\0';				
				printf("+BRX:len=%d,%s\n\r", recv_count, buffer);
							
			} else {
				OSTimeDly(10);	
			}
        } else {
		    OSTimeDly(50);		
		}
    }
}


