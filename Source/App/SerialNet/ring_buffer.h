/*
 * =====================================================================================
 *
 *       Filename:  ring_buffer.h
 *
 *    Description:  Head file of ring buffer
 *
 *        Version:  0.0.1
 *        Created:  2015/6/5 9:59:02
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

#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include    <stdbool.h>

typedef struct
{
    unsigned int  size;
    unsigned int  head;
    unsigned int  tail;
    unsigned char*  buffer;
} ring_buffer_t;


#ifndef MIN
#define MIN(x,y)  (((x) < (y) ? (x) : (y)))
#endif /* ifndef MIN */


void ring_buf_clear(ring_buffer_t *ring);
bool ring_buf_empty(ring_buffer_t *ring);
bool ring_buf_full(ring_buffer_t *ring);

int ring_buf_cnt(ring_buffer_t *ring);
int ring_buf_space(ring_buffer_t *ring);
int ring_buf_cnt_to_end(ring_buffer_t *ring);
int ring_buf_space_to_end(ring_buffer_t *ring);

int ring_buf_write_char(ring_buffer_t *cb, char data);
int ring_buf_write(ring_buffer_t *cb, char *buf, int count);
int ring_buf_read(ring_buffer_t *cb, char *buf, int count);

int ring_buf_alloc(ring_buffer_t* ring, unsigned int data_len);
void ring_buf_free(ring_buffer_t* ring);

#endif 


