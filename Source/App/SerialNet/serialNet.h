/*
******************************************************************************
**
** Project     : 
** Description :    SerialNet header file
** Author      :    linhui                             
** Date        : 
**    
** UpdatedBy   : 
** UpdatedDate : 
**
******************************************************************************
*/

#ifndef __SERIALNET_H__
#define __SERIALNET_H__


/* define UART irq enable MACRO */
#define HW_UART_IRQ_SUPPORT

/**********************************************************/
/*************** Serial to wifi interface *****************/
/**********************************************************/

/* Receive one byte from uart, called by BSP_UartISR interrupt
 * function. */
void uart_data_recv(char Dummy);

/* When the smart config finish, serialNet mode will called it
 * to finish internal task.
 **/
void ResponseSmartconfig(void);

/* SerialNet SDK entry function */
int TestSerialToWifi(void);	 

int init_default_data(void);

#endif


