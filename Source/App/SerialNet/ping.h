#ifndef __PING_H__
#define __PING_H__

#include "lwip/raw.h"
#include "lwip/icmp.h"

#include "common.h"

/* define system now function, which will get the current system tick. */
#define system_now() OSTimeGet()

//! @brief Typedef for callback function.
//!
//! If a valid echo reply is received, this callback will be invoked. The @a echo
//! parameter will be a valid pointer. If the echo timed out, the callback will
//! be invoked will @a echo set to NULL.
typedef void (*ping_callback_t)(const struct icmp_echo_hdr * echo, const ip_addr_t *addr, int elapsed);

int ping_init(int count);
void ping_send_to(ip_addr_t * ping_target, ping_callback_t callback);

#endif /* __PING_H__ */

