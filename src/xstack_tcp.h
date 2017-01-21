/**
 * @addtogroup TCP
 * @{
 */

#ifndef XSTACK_TCP_H
#define XSTACK_TCP_H

#include <stdint.h>

#include "xstack_ip.h"

/**
 * TCP Header.
 */
struct tcp {
    uint16_t tcp_sport;
    uint16_t tcp_dport;
    uint16_t tcp_seqno;
    uint16_t tcp_ackno;
    uint16_t tcp_flags;
    uint16_t tcp_win_size;
    uint16_t tcp_csum;
    uint16_t tcp_urg_p;
    uint8_t opt[0];
};

#endif /* XSTACK_TCP_H */

/**
 * @}
 */
