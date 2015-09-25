#ifndef __SOCKETCOM_H__
#define __SOCKETCOM_H__

#include "common.h"


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

