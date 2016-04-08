/*
 * =====================================================================================
 *
 *       Filename:  ring_buffer.c
 *
 *    Description:  ring buffer process interface
 *
 *        Version:  0.0.1
 *        Created:  2015/6/5 9:32:44
 *       Revision:  none
 *
 *         Author:  Lin Hui (Link), linhui.568@163.com
 *   Organization:  Nufront
 *
 *--------------------------------------------------------------------------------------          
 *       ChangLog:
 *        version    Author      Date          Purpose
 *        0.0.1      Lin Hui    2015/6/5      
 *
 * =====================================================================================
 */

#include "common.h"
#include "ring_buffer.h"

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ring_buf_clear
 *  Description:  Clear ring buffer
 *		 Return:  1 if the ring buffer is empty, otherwise return 0.
 *         Note:  Clear the ring buffer only setting the write position and read 
 *				position to zero but not clear the content of the buffer.
 * =====================================================================================
 */ 
void ring_buf_clear(ring_buffer_t *ring)
{
	ring->tail = ring->head = 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ring_buf_empty
 *  Description:  check if the ring buffer is empty
 *		 Return:  1 if the ring buffer is empty, otherwise return 0.
 *         Note:  
 * =====================================================================================
 */  
bool ring_buf_empty(ring_buffer_t *ring)
{
	return (ring->tail == ring->head);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ring_buf_full
 *  Description:  check if the ring buffer is full
 *		 Return:  1 if the ring buffer is full, otherwise return 0.
 *         Note:  
 * =====================================================================================
 */
bool ring_buf_full(ring_buffer_t *ring)
{
	return (((ring->tail + 1) % ring->size) == ring->head);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ring_buf_cnt
 *  Description:  Return count in buffer..
 *         Note:  
 * =====================================================================================
 */
int ring_buf_cnt(ring_buffer_t *ring)
{
	if (ring->tail >= ring->head) {
		return ring->tail - ring->head ;	
	} else {
		return ring->size + ring->tail - ring->head;
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ring_buf_space
 *  Description:  Get the free space of ring buffer.
 *         Note:  Return space available, 0..size-1.  We always leave one free char
 *				as a completely full buffer has tail == read, which is the same as empty.
 * =====================================================================================
 */
int ring_buf_space(ring_buffer_t *ring)
{
	if (ring->head > ring->tail) {
		return ring->head - ring->tail - 1;	
	} else {
		return ring->size - 1 + ring->head - ring->tail;
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ring_buf_cnt_to_end
 *  Description:  Return count up to the end of the buffer.Carefully avoid.	accessing 
 *				tail and read more than once, so they can change underneath us without
 *				returning inconsistent results.
 *         Note:  
 * =====================================================================================
 */
int ring_buf_cnt_to_end(ring_buffer_t *ring)
{
	if (ring->tail >= ring->head) {
		return (ring->tail - ring->head);	
	} else {
		return (ring->size - ring->head);
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ring_buf_space_to_end
 *  Description:  Return space available up to the end of the buffer.
 *       Return:  return the real length.
 *         Note:  
 * =====================================================================================
 */
int ring_buf_space_to_end(ring_buffer_t *ring)
{
	if (ring->head > ring->tail) {
		return (ring->head - ring->tail - 1);	
	} else {
		return ((ring->size - 1) - ring->tail + ((ring->head > 0) ? 1 : 0));
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ring_buf_write_char
 *  Description:  Copy a char from a user buffer and put it into the circular buffer.  
 *				Restrict to the amount of space available.
 *       Return:  1, success; otherwise return 0.
 *         Note:  
 * =====================================================================================
 */
int ring_buf_write_char(ring_buffer_t *cb, char data)
{
	ring_buffer_t *temp = cb;
    if (!ring_buf_full(temp)) {
        cb->buffer[temp->tail] = data;

        cb->tail++;
        if (temp->tail >= temp->size)
            temp->tail -= temp->size;
        return 1;
    }

    return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ring_buf_write
 *  Description:  Copy data from a user buffer and put it into the circular buffer.  
 *				Restrict to the amount of space available.
 *       Return:  the number of bytes written.
 *         Note:  The data may be only partially written if there isn't room in the 
 *				buffer for the count bytes that were requested.
 * =====================================================================================
 */
int ring_buf_write(ring_buffer_t *cb, char *buf, int count)
{
    int c, ret = 0;

    while (1) {
        c = ring_buf_space_to_end(cb);
        if (count < c)
            c = count;

        if (c <= 0)
            break;		

        memcpy(cb->buffer + cb->tail, (unsigned char*)buf, c);

        cb->tail += c;
        if (cb->tail >= cb->size)
            cb->tail -= cb->size;

        buf += c;
        count -= c;
        ret+= c;
    }
    return ret;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ring_buf_read
 *  Description:  Get data from the ring buffer and copy to the given buffer. Restrict 
 *				to the amount of data available.
 *       Return:  number of bytes really readed.
 *         Note:  The number of bytes really readed may be less than the amount requested
 *				in the count argument to the method if the amount of data available is 
 *				less than the amount requested.
 * =====================================================================================
 */
int ring_buf_read(ring_buffer_t *cb, char *buf, int count)
{
    int c, ret = 0;

    while (1) {
        c = ring_buf_cnt_to_end(cb);
        if (count < c)
            c = count;

        if (c <= 0)
            break;

        memcpy((unsigned char*)buf, cb->buffer + cb->head, c);

        cb->head += c;
        if (cb->head >= cb->size)
            cb->head -= cb->size;

        buf += c;
        count -= c;
        ret+= c;
    }

    return ret;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ring_buf_alloc
 *  Description:  Initialize and Alloc a ring buffer. 
 *         Note:  
 * =====================================================================================
 */
int ring_buf_alloc(ring_buffer_t* ring, unsigned int data_len)
{
	ring->buffer = OSMMalloc(data_len);
	ring->size = data_len;
	ring->head = ring->tail = 0;		
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ring_buf_free
 *  Description:  Free a ring buffer. 
 *         Note:  
 * =====================================================================================
 */
void ring_buf_free(ring_buffer_t* ring)
{
	OSMFree(ring);
	ring_buf_clear(ring);
}
