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


/* When the smart config finish, serialNet mode will called it
 * to finish internal task.
 **/
void ResponseSmartconfig(void);

/* SerialNet SDK entry function */
void TestSerialToWifi(void * pParam);	 

int init_default_data(void);

#endif


