

#include "socketCom.h"

extern SYS_EVT_ID link_status;

/* wifi connect status struct */
CONN_STATUS WifiConnStatus;

VOID FormatPrintf(int len)
{
	int i = 0;

	for(i = 0; i < len; i++)
	{
		//printf("%c",uart_send_data[i]);
		BSP_UartPutcPolled(uart_send_data[i]);
	}

}

/*
 * When network data comming, start to send data to uart, 
 * limitation: uart buffer is less than 2k.
 **/
VOID UartSendThread(VOID *arg)
{
	while (1) {
		if (UserParam.atMode == DATA_MODE) {	
			if(uart_sendStart == 0 && uart_hasData == 1) {
				uart_sendStart = 1;

				FormatPrintf(uart_send_len);
//				printf(uart_send_data);
				uart_sendStart = 0;
				uart_hasData = 0;
			} else {
				OSTimeDly(1);
			}
		} else {
			OSTimeDly(100);
		}
	}
}

#define UDP_ONEFRAME_SEND_SWITCH		(1)

void UDPSendTask(unsigned char type)
{	
	int sendcount;
	struct sockaddr_in local_addr;
	char *pdata;

#if UDP_ONEFRAME_SEND_SWITCH
	UINT32	data_len = 0;
//	UINT32	send_fail_cnt = 0;
//    OS_CPU_SR  cpu_sr;
#endif

	while (WifiConnStatus.connStatus == CONNECT_STATUS_FAIL) {
		OSTimeDly(100);	
	}

	if (type == SOCK_CLIENT) {
		memcpy(&local_addr, &WifiConnStatus.server_addr, sizeof(struct sockaddr_in));
				
	} else if (type == SOCK_SERVER) {
		memcpy(&local_addr, &WifiConnStatus.client_addr, sizeof(struct sockaddr_in));
	}

	while (1) {
		if (UserParam.atMode == DATA_MODE) {	
			if (WifiConnStatus.sockfd != -1) {
			
				if (net_sendStart == 1) {
#if UDP_ONEFRAME_SEND_SWITCH
					data_len = net_send_len;
					pdata = net_send_data;
					
					/* close system interrupt until send data finish */
					//OS_ENTER_CRITICAL();	
					
					/* send data to network as every frame user's set in "AT+UARTFL" */
					while (data_len > 0) {
						sendcount = sendto(WifiConnStatus.sockfd, pdata, 
							(data_len > UserParam.frameLength ? UserParam.frameLength : data_len), 
							0, (struct sockaddr *)&local_addr, sizeof(struct sockaddr));
						if (sendcount < 0) {
								continue;							
						} else {
							pdata+= sendcount;
							data_len -= sendcount;
						}
					}

					//OS_EXIT_CRITICAL();
#else
					sendcount = sendto(WifiConnStatus.sockfd, net_send_data, net_send_len, 
						0, (struct sockaddr *)&local_addr, sizeof(struct sockaddr));
					if (sendcount < 0) {
						printf("+ERROR= %d\n\r", INVALID_SEND);
					}	
#endif
					net_sendStart = 0;
					net_send_len = 0;
				} else {
					OSTimeDly(3);
				}
			} else {
				net_sendStart = 0;
				net_send_len = 0;
				uart_recvEnd = 0;
				uart_rec_len = 0;
				OSTimeDly(10);
				break;
			} 
		} else {
			if (WifiConnStatus.sockfd >= 0) {
				lwip_close(WifiConnStatus.sockfd);
				WifiConnStatus.sockfd = -1;
			}
			break;
		}
	}
}

void UDPRecvTask(unsigned char type)
{
	int ret;
	int bytes_read;
	int count = 0;
	struct timeval timeout;	
    
	u32_t addr_len = sizeof(struct sockaddr);
	char *recv_data;
	recv_data = OSMMalloc(g_RecvBufSize);

	timeout.tv_sec = 3;   /* receive data timeout: 3 seconds */
    timeout.tv_usec = 0;

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

		ret = setsockopt(WifiConnStatus.sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		if (ret == -1) {
		   	printf("+ERROR= %d\n\r", INVALID_TCP_SETOPT);
		}

		/* bing server */
		if (bind(WifiConnStatus.sockfd, (struct sockaddr *)&WifiConnStatus.server_addr,
			   sizeof(struct sockaddr)) == -1) {
			printf("+ERROR= %d\n\r", INVALID_BIND);
			goto END;
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
					/* When device switch to AT mode, return this interface. */
					if (UserParam.atMode == AT_MODE) {					
						if (WifiConnStatus.sockfd >= 0) {
							lwip_close(WifiConnStatus.sockfd);
							WifiConnStatus.sockfd = -1;
						}
						goto END;					
					}

					continue;
				}
				
				/* when client connect local server first, save the client connect attributation
				 * and launch send task, else filter it 
				 **/
				if ((type == SOCK_SERVER) && (count <= 0)) {
					WifiConnStatus.connStatus = CONNECT_STATUS_OK;
					count++;
				}
	
				recv_data[bytes_read] = '\0';
				while (1) {
					if (uart_sendStart == 0 && uart_hasData == 0) {
						memcpy((INT8U *)uart_send_data, (INT8U *)recv_data, bytes_read);
						uart_send_len = bytes_read;
						uart_hasData = 1;
						break;
					} else {
						OSTimeDly(1);
					}
				} 
			} else {
				break;
			}
		} else {
			if (WifiConnStatus.sockfd >= 0) {
				lwip_close(WifiConnStatus.sockfd);
				WifiConnStatus.sockfd = -1;
			}
			break;
		}
	}

END:
	WifiConnStatus.connStatus = CONNECT_STATUS_FAIL;
	uart_recvEnd = 0;	
	uart_rec_len = 0;
	net_sendStart = 0;
	net_send_len = 0;

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
	int snd;
	int socketFd = -1;
	UINT8  FailCnt = 0;


#if TCP_ONEFRAME_SEND_SWITCH
	char *pdata;
	int sendcount;
	UINT32	data_len = 0;
	UINT32	send_fail_cnt = 0;
#endif

//	memset(net_send_data, '\0', g_RecvBufSize);

	while (WifiConnStatus.connStatus == CONNECT_STATUS_FAIL) {
		OSTimeDly(100);	
	}
	

	if (type == SOCK_CLIENT) {
		socketFd = WifiConnStatus.sockfd;
	} else if (type == SOCK_SERVER) {
		socketFd = WifiConnStatus.connectFd;
	}
	
	while (1) {
		if (UserParam.atMode == DATA_MODE) {
			if (socketFd != -1) {

				if(WifiConnStatus.connStatus == CONNECT_STATUS_FAIL){
					printf("+ERROR= %d\n\r", INVALID_TCP_CONNECT);
					OSTimeDly(100);
					return;
				}

			  	if (net_sendStart == 1) {
#if TCP_ONEFRAME_SEND_SWITCH
					data_len = net_send_len;
					pdata = net_send_data;
					
					/* close system interrupt until send data finish */
					//OS_ENTER_CRITICAL();	
					
					/* send data to network as every frame user's set in "AT+UARTFL" */
					while (data_len > 0) {
						sendcount = send(socketFd, pdata, 
							(data_len > UserParam.frameLength ? UserParam.frameLength : data_len), 0);
						
						if(UserParam.atMode != DATA_MODE){
								return;
						}
							
						if (sendcount < 0) {
							printf("+ERROR= %d\n\r", INVALID_SEND);
							send_fail_cnt++;

							/* if send data error more than 5 times, start send task again */
							if (send_fail_cnt >= 5) {
								net_sendStart = 0;
								net_send_len = 0;
								//OS_EXIT_CRITICAL();
								return;							
							}
						} else {
							pdata+=sendcount;
							data_len -= sendcount;
						}
					}

					//OS_EXIT_CRITICAL();
#else
					snd = send(socketFd, net_send_data, net_send_len, 0);
					if (snd < 0) {
						printf("+ERROR= %d\n\r", INVALID_SEND);
						OSTimeDly(10);                
						FailCnt++;

						if(FailCnt == 10)
						{
							net_sendStart = 0;
							net_send_len = 0;
							uart_recvEnd = 0;
							uart_rec_len = 0;
							
							WifiConnStatus.connStatus = CONNECT_STATUS_FAIL;
							return;	
						}
				    } else {
						FailCnt = 0;
					}			
#endif
					net_sendStart = 0;						
					net_send_len = 0;
				} else {
			  		OSTimeDly(1);
			  	}
			} else {
				net_sendStart = 0;
				net_send_len = 0;
				uart_recvEnd = 0;
				uart_rec_len = 0;
				OSTimeDly(10);
				return;
								
			}
		} else {	/* command mode */
			return;
		}
	}
}


void TCPRecvTask(unsigned char type)
{
	int socketFd;
	fd_set fdR;
	int select_ret;

	int ret;
	int tmp = 1;
	int bytes_read;
	char *recv_data;
	struct timeval timeout;

	int keepAlive = 1; // 开启keepalive属性
	int keepIdle = 30; // 如该连接在30秒内没有任何数据往来,则进行探测 
	int keepInterval = 5; // 探测时发包的时间间隔为5 秒
	int keepCount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
	
	u32_t sin_size = sizeof(struct sockaddr_in);
	recv_data = OSMMalloc(g_RecvBufSize);

	timeout.tv_sec = 3;   /* receive data timeout: 3 seconds */
    timeout.tv_usec = 0; 


	/* Create socket and connect to server */
	while (1) {	
		if (UserParam.atMode == DATA_MODE) {
			if (WifiConnStatus.sockfd >= 0) {
				lwip_close(WifiConnStatus.sockfd);
				WifiConnStatus.sockfd = -1;
			}
			OSTimeDly(30);

			/* create socket:type is SOCK_STREAM, TCP */
			if ((WifiConnStatus.sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			   printf("+ERROR= %d\n\r", INVALID_TCP_SOCKET);
			   goto END;
			}
		
			if (type == SOCK_CLIENT) {
			    memset(&(WifiConnStatus.server_addr), 0, sizeof(WifiConnStatus.server_addr));
			    WifiConnStatus.server_addr.sin_family = AF_INET;
			    WifiConnStatus.server_addr.sin_port = htons(WifiConnStatus.socketPort);
			    WifiConnStatus.server_addr.sin_addr.s_addr = WifiConnStatus.remoteIp;
			    memset(&(WifiConnStatus.server_addr.sin_zero), 0, sizeof(WifiConnStatus.server_addr.sin_zero));

				ret = setsockopt(WifiConnStatus.sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
				//ret = setsockopt(WifiConnStatus.sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
				if (ret == -1) {
				   	printf("+ERROR= %d\n\r", INVALID_TCP_SETOPT);
				}

				printf("TCP client connecting on port %d...\n", WifiConnStatus.socketPort);
					
				if (connect(WifiConnStatus.sockfd, (struct sockaddr *)&WifiConnStatus.server_addr,
				        sizeof(struct sockaddr)) == -1)	{
					printf("+ERROR= %d\n\r", INVALID_TCP_CONNECT);
				   	OSTimeDly(100);

				} else {
					if(UserParam.atMode != DATA_MODE)
					break;

				    printf("+OK:Connected Server\n");
					break;
				}
						
			} else if (type == SOCK_SERVER) {
	
				memset(&(WifiConnStatus.server_addr), 0, sizeof(WifiConnStatus.server_addr));
				WifiConnStatus.server_addr.sin_family = AF_INET;
				WifiConnStatus.server_addr.sin_port = htons(WifiConnStatus.socketPort);
				WifiConnStatus.server_addr.sin_addr.s_addr = INADDR_ANY;
				memset(&(WifiConnStatus.server_addr.sin_zero), 0, sizeof(WifiConnStatus.server_addr.sin_zero));

				ret = setsockopt(WifiConnStatus.sockfd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));
				if (ret == -1) {
				   	printf("+ERROR= %d\n\r", INVALID_TCP_SETOPT);
				}
		
				/* bing server */
				if (bind(WifiConnStatus.sockfd, (struct sockaddr *)&WifiConnStatus.server_addr,
					   sizeof(struct sockaddr)) == -1) {
					printf("+ERROR= %d\n\r", INVALID_BIND);
					OSTimeDly(100);
					continue;
				}
			   			
			   	if (listen(WifiConnStatus.sockfd, 5) == -1) {
			       		printf("+ERROR= %d\n\r", INVALID_TCP_LISTEN);
					OSTimeDly(100);
			       		continue;
			   	}

			    timeout.tv_sec = 7;   /* receive data timeout: 7 seconds */
			    timeout.tv_usec = 0;
			    FD_ZERO(&fdR);
			    FD_SET(WifiConnStatus.sockfd, &fdR);
			    printf("TCP server waiting on port %d...\n", WifiConnStatus.socketPort);

			    switch (select(WifiConnStatus.sockfd + 1, &fdR, NULL, NULL, &timeout)) {
				    case -1:
					    select_ret = 1;
					    printf("error\n\r");
					    goto END;
				    case 0:
					    select_ret = -1;
					    //                        printf("timeout\n\r");
					    break;
				    default:
					    select_ret = 0;
					    if (FD_ISSET(WifiConnStatus.sockfd, &fdR)) {
						    break;
					    }
			    }
			    if (select_ret == -1)   /* timeout */
				continue;

				WifiConnStatus.connectFd = accept(WifiConnStatus.sockfd, 
						(struct sockaddr *)&WifiConnStatus.client_addr, &sin_size);

				ret = setsockopt(WifiConnStatus.connectFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
				if (ret == -1) {
				   	printf("+ERROR= %d\n\r", INVALID_TCP_SETOPT);
				}

				ret = setsockopt(WifiConnStatus.connectFd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
				if (ret == -1) {
				   	printf("+ERROR= %d\n\r", INVALID_TCP_SETOPT);
				}

				ret = setsockopt(WifiConnStatus.connectFd, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
				if (ret == -1) {
				   	printf("+ERROR= %d\n\r", INVALID_TCP_SETOPT);
				}

				ret = setsockopt(WifiConnStatus.connectFd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
				if (ret == -1) {
				   	printf("+ERROR= %d\n\r", INVALID_TCP_SETOPT);
				}

				ret = setsockopt(WifiConnStatus.connectFd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));
				if (ret == -1) {
				   	printf("+ERROR= %d\n\r", INVALID_TCP_SETOPT);
				}
				
				printf("+OK:DEV Connected\n\r");
				break;												
			}					
			
		} else {
			goto END;
		}
	}

	/*
	 * After socket create, set connect success, resume send task thread.
	 **/		
	if (type == SOCK_CLIENT) {
		socketFd = WifiConnStatus.sockfd;
	} else if (type == SOCK_SERVER) {
		socketFd = WifiConnStatus.connectFd;
	}
	OSTimeDly(20);
	WifiConnStatus.connStatus = CONNECT_STATUS_OK;	

	while (1) {
		if (UserParam.atMode == DATA_MODE) {
			if (socketFd != -1) {

				if(WifiConnStatus.connStatus == CONNECT_STATUS_FAIL)
				{
					goto END;
				}


				bytes_read = recv(socketFd, recv_data, g_RecvBufSize, 0);					
				if (bytes_read <= 0) {	
					/* When device switch to AT mode, return this interface. */
					if (UserParam.atMode == AT_MODE) {					
						//printf("+ERROR= %d\n\r", INVALID_RECV);
						goto END;					
					}
					if(bytes_read == 0){
						WifiConnStatus.connStatus = CONNECT_STATUS_FAIL;
						goto END;
					}else{
						continue;
					}
				}
				WifiConnStatus.connStatus = CONNECT_STATUS_OK;
				recv_data[bytes_read] = '\0'; 
				while (1) {
					if (uart_sendStart == 0 && uart_hasData == 0) {
						memcpy((INT8U *)uart_send_data, (INT8U *)recv_data, bytes_read);
						uart_send_len = bytes_read;
						uart_hasData = 1;                                                                                                                                                                                                                                                                                                                                                               
						break;
					} else {
						OSTimeDly(1);
					}
				} 
			} else {
				break;
			}
		
		} else {
			goto END;
		}
	}

END:
	WifiConnStatus.connStatus = CONNECT_STATUS_FAIL;
	uart_recvEnd = 0;	
	uart_rec_len = 0;
	net_sendStart = 0;
	net_send_len = 0;
	
	if(WifiConnStatus.connectFd >= 0)
	{
		lwip_close(WifiConnStatus.connectFd);
		WifiConnStatus.connectFd = -1;
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
			switch (WifiConnStatus.socketProtocol) {
				case 0:	/* UDP connect */
					if (WifiConnStatus.socketType == SOCK_CLIENT) {
						UDPSendTask(SOCK_CLIENT);
					} else if (WifiConnStatus.socketType == SOCK_SERVER) {
						UDPSendTask(SOCK_SERVER);
					} 
					break;

				case 1:	/* TCP connect */
					if (WifiConnStatus.socketType == SOCK_CLIENT) {
						TCPSendTask(SOCK_CLIENT);
					} else if (WifiConnStatus.socketType == SOCK_SERVER) {
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
			switch (WifiConnStatus.socketProtocol) {
				case 0:	/* UDP connect */
					if (WifiConnStatus.socketType == SOCK_CLIENT) {
						UDPRecvTask(SOCK_CLIENT);	
					
					} else if (WifiConnStatus.socketType == SOCK_SERVER) {
					   	UDPRecvTask(SOCK_SERVER);

					}
					break;

				case 1:	/* TCP connect */
					if (WifiConnStatus.socketType == SOCK_CLIENT) {
						TCPRecvTask(SOCK_CLIENT);					
					} else if (WifiConnStatus.socketType == SOCK_SERVER) {
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


