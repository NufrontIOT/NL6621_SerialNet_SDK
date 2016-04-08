/*
 * =====================================================================================
 *
 *       Filename:  nl6621_uart.c
 *
 *    Description:  nl6621 uart driver file
 *
 *        Version:  0.0.1
 *        Created:  2015/7/1 13:30:44
 *       Revision:  none
 *
 *         Author:  Lin Hui (Link), linhui.568@163.com
 *   Organization:  Nufront
 *
 *--------------------------------------------------------------------------------------          
 *       ChangLog:
 *        version    Author      Date          Purpose
 *        0.0.1      Lin Hui    2015/7/1      
 *
 * =====================================================================================
 */
#include "common.h"
#include "nl6621_uart.h"
#include "ring_buffer.h"

ring_buffer_t uartrxbuf;
static unsigned char atHead[2];
static char *pCmdLine;
extern char uart_rec_atcmd[MAX_AT_RECV_SIZE];
extern UINT8 	net_sendStart;
UINT8 uart_send_flag = UART_START;
NST_LOCK * recv_lock;	/* receive lock */

/* store data receive from uart, user can change the data store address.
 * */
UINT8	end_sendFlags = 0;

at_stateType  at_state;

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  uart_data_recv
 *  Description:  Interface of uart data receive. 
 *         Note:
 * =====================================================================================
 */
void uart_data_recv(char Dummy)
{

	/* when in data mode ,'+++' means escape */
	if(UserParam.atMode == DATA_MODE){

		NST_AQUIRE_LOCK(recv_lock);
		ring_buf_write_char(&uartrxbuf, (char)Dummy);
		NST_RELEASE_LOCK(recv_lock);

		if(net_sendStart == SEND_DONE)
		{
			/*reset timer0*/
	        *Tmr0Ctl = TMR_INT_MASK;
	        *Tmr0Load =40000*UserParam.frameGap;    
	        *Tmr0Ctl = (~TMR_INT_MASK) |TMR_ENA | TMR_USER_DEFINE_MODE;
			net_sendStart = SEND_START;
		}

		if(ring_buf_cnt(&uartrxbuf) >= UserParam.frameLength && ring_buf_full(&uartrxbuf) != 0)
		{
			OSSemPost(sendSwitchSem);
		}

		if(ring_buf_cnt(&uartrxbuf) > UART_RECV_SIZE_STOP && uart_send_flag == UART_START && (WifiConnStatus.sockfd >= 0 || Get_MaxFd() > 0))
		{
			uart_send_flag = UART_STOP;
			//BSP_UartPutcPolled(stop);
			BSP_GPIOSetValue(UART_RTS, GPIO_LOW_LEVEL);
			
			/*reset timer0*/
	        *Tmr0Ctl = TMR_INT_MASK;
	        *Tmr0Load =40000*UserParam.frameGap;    
	        *Tmr0Ctl = (~TMR_INT_MASK) |TMR_ENA | TMR_USER_DEFINE_MODE;
			net_sendStart = SEND_START;
				
		}

	//	if(strcmp(&Dummy,"+") == 0){
	    if(Dummy == '+') {
		//	printf("+\n\r");
			end_sendFlags++;

			if(end_sendFlags == END_SEND_TRUE){
				OSSemPost(modeSwitchSem);	
			}

		}else{
			if(end_sendFlags == END_SEND_TRUE
			&& (Dummy == '\r' || Dummy == '\n'))
			{
				end_sendFlags = END_SEND_TRUE; 	
			}else{
				end_sendFlags = END_SEND_FALSE;
			}
		}
//
//		if(end_sendFlags == END_SEND_TRUE && strcmp(&Dummy,"+") != 0){
//			end_sendFlags = END_SEND_FALSE; 
//		}
	} else {
		switch(at_state)
		{
			case at_statIdle:
				atHead[0] = atHead[1];
				atHead[1] = Dummy;
				
				if((memcmp(atHead, "AT", 2) == 0) || (memcmp(atHead, "at" , 2) == 0))
				{
					at_state = at_statRecving;
					pCmdLine = uart_rec_atcmd;
	        		atHead[1] = 0x00;
				}
				else if(Dummy == '\n') //only get enter
			    {
//			        printf("\r\nERROR\r\n");
			    }
			    break;
			
			case at_statRecving: //push receive data to cmd line
				*pCmdLine = Dummy;
				if(Dummy == '\n')
				{
					pCmdLine--;
					*pCmdLine = '\0';
					at_state = at_statProcess;
				}
				else if(pCmdLine >= &uart_rec_atcmd[MAX_AT_RECV_SIZE - 1])
				{
					at_state = at_statIdle;
				}
				pCmdLine++;
				break;
				
			case at_statProcess: //process data
				if(Dummy == '\n')
				{
					printf("\r\nbusy p...\r\n");
				}
				break;
			
			default:
				if(Dummy == '\n')
				{
				}
				break; 
		}
	}
			
}



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  uart_init
 *  Description:  Init uart resource, sended and received   
 *         Note:
 * =====================================================================================
 */
void uart_init(void)
{
	/* Init uart receive ring buffer */
	ring_buf_alloc(&uartrxbuf, UART_RECV_BUF_SIZE);

	NST_ALLOC_LOCK(&recv_lock);

}		/* -----  end of function uart_init  ----- */

