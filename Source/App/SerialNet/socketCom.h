#ifndef __SOCKETCOM_H__
#define __SOCKETCOM_H__

#include "common.h"

typedef int   int32;
typedef unsigned short uint16;

typedef struct lanserver_t
{
	fd_set readfd;
	int32 tcpServerFd;

	struct{
		int32 fd;
		int32 timeout;
		int32 isLogin;
	}tcpClient[5];
}lanserver, *planserver;

#define sockaddr_t sockaddr_in
#define LAN_TCPCLIENT_MAX           5       /* max 5 tcp socket */
#define LAN_CLIENT_LOGIN_SUCCESS        0
#define LAN_CLIENT_LOGIN_FAIL           1
#define SOCKET_TCPSOCKET_BUFFERSIZE    (1*1024)
#define INVALID_SOCKET  (-1)
#define SOCKET_ERROR    (-1)

VOID UartSendThread(VOID *arg);

void UDPSendTask(unsigned char type);
void UDPRecvTask(unsigned char type);

void TCPSendTask(unsigned char type);
void TCPRecvTask(unsigned char type);

VOID SendThread(VOID *arg);
VOID RecvThread(VOID *arg);

VOID RespondUdpBroadcast(VOID);

VOID BCTRecvThread(VOID * arg);

#endif

