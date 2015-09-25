/**
 * @file
 * Ping sender module
 *
 */

/*
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 */

/** 
 * This is an example of a "ping" sender (with raw API and socket API).
 * It can be used as a start point to maintain opened a network connection, or
 * like a network "watchdog" for your device.
 *
 */

#include "lwip/opt.h"

#if LWIP_RAW /* don't build if not configured for use in lwip/opts.h */

#include "ping.h"

#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/timers.h"
#include "lwip/inet_chksum.h"

/** ping delay - in milliseconds */
#ifndef PING_DELAY
#define PING_DELAY     1000
#endif

/** ping identifier - must fit on a u16_t */
#ifndef PING_ID
#define PING_ID        0xAFAF
#endif

/** ping additional data size to include in the packet */
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif

/* ping variables */
static ip_addr_t s_ping_target;
static u16_t ping_seq_num;
static u16_t ping_recv_num;
static u16_t ping_count;
static u32_t ping_time;
static ping_callback_t s_ping_callback = NULL;

static struct raw_pcb *ping_pcb;
static int ping_flag = 0;

/** Prepare a echo ICMP request */
static void ping_prepare_echo( struct icmp_echo_hdr *iecho, u16_t len)
{
    size_t i;
    size_t data_len = len - sizeof(struct icmp_echo_hdr);

    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id     = PING_ID;
    iecho->seqno  = htons(++ping_seq_num);

    /* fill the additional data buffer with some data */
    for(i = 0; i < data_len; i++) {
        ((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
    }

    iecho->chksum = inet_chksum(iecho, len);
}

/* Ping using the raw ip */
static u8_t ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, ip_addr_t *addr)
{
    struct icmp_echo_hdr *iecho;
	int elapsed;

    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(pcb);
    LWIP_UNUSED_ARG(addr);
    LWIP_ASSERT("p != NULL", p != NULL);

    if ((p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))) &&
            pbuf_header( p, -PBUF_IP_HLEN) == 0) {
        iecho = (struct icmp_echo_hdr *)p->payload;
        elapsed = system_now() - ping_time;

        if (s_ping_callback) {
            s_ping_callback(iecho, addr, elapsed);
        }

        if ((iecho->id == PING_ID) && (iecho->seqno == htons(ping_seq_num))) {
            
			ping_recv_num++;
			printf("Reply from %s: ", inet_ntoa(addr->addr));
  			printf("bytes=32 time=%dms\tTTL=%d\n", (elapsed), pcb->ttl);

            /* do some ping result processing */
            pbuf_free(p);
            return 1; /* eat the packet */
        }
    }

    return 0; /* don't eat the packet */
}

static void ping_send(struct raw_pcb *raw, ip_addr_t *addr)
{
    struct pbuf *p;
    struct icmp_echo_hdr *iecho;
    size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;

    // Save target address.
    s_ping_target = *addr;

    LWIP_ASSERT("ping_size <= 0xffff", ping_size <= 0xffff);

    p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
    if (!p) {
        return;
    }

    if ((p->len == p->tot_len) && (p->next == NULL)) {
        iecho = (struct icmp_echo_hdr *)p->payload;

        ping_prepare_echo(iecho, (u16_t)ping_size);

        //printf("ping: send seq=%d\n", ntohs(iecho->seqno));

        raw_sendto(raw, p, addr);
        ping_time = system_now();
    }
    pbuf_free(p);
}

static void ping_timeout(void *arg)
{
    struct raw_pcb *pcb = (struct raw_pcb*)arg;
    ip_addr_t ping_target = s_ping_target; //PING_TARGET;

    LWIP_ASSERT("ping_timeout: no pcb given!", pcb != NULL);

    if (s_ping_callback) {
        s_ping_callback(NULL, NULL, 0);
    }

	if(ping_count != 0){
		if (ping_seq_num >= ping_count) {
			printf("Ping statistics for %s:\n\r", inet_ntoa(ping_target.addr));
			printf("\tPackets:Sent=%d, Received=%d, lost=%d(%d%% loss)\n\r", 
				ping_seq_num, ping_recv_num, ping_seq_num - ping_recv_num,
				((ping_seq_num - ping_recv_num) * 100) / ping_seq_num);

			printf("+OK:PING\n\r");
			ping_flag = 0;
			raw_remove(ping_pcb);
			return;
		}
	}

    ping_send(pcb, &ping_target);

    sys_timeout(PING_DELAY, ping_timeout, pcb);
}

void ping_send_to(ip_addr_t * ping_target, ping_callback_t callback)
{
	printf("Pinging from %s: with 32 bytes of data:\n\n\r", 
		inet_ntoa(ping_target->addr));	

    LWIP_ASSERT("ping_pcb != NULL", ping_pcb != NULL);
    s_ping_callback = callback;
    ping_send(ping_pcb, ping_target);
}

static void ping_raw_init(void)
{
	ping_recv_num = 0;
    ping_seq_num = 0;

	ping_pcb = raw_new(IP_PROTO_ICMP);
    LWIP_ASSERT("ping_pcb != NULL", ping_pcb != NULL);

    s_ping_callback = NULL;
    raw_recv(ping_pcb, ping_recv, NULL);
    raw_bind(ping_pcb, IP_ADDR_ANY);
    sys_timeout(PING_DELAY, ping_timeout, ping_pcb);
}

int ping_init(int count)
{
	/* if the ping task is running, filter it */
	if (ping_flag == 0) {
		ping_flag = 1;
		ping_count = count;
		ping_raw_init();
		return 0;
	} else {
		return -1;
	}					    
}

#endif /* LWIP_RAW */
