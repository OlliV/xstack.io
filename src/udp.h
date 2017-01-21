/**
 * Xstack UDP service.
 * @addtogroup UDP
 * @{
 */

#ifndef XSTACK_UDP_H
#define XSTACK_UDP_H

#include <stdint.h>

#include "linker_set.h"
#include "xstack_in.h"

#define UDP_MAXLEN  65507

/**
 * Type for an UDP port number.
 */
typedef uint16_t udp_port_t;

/**
 * UDP packet header.
 */
struct udp_hdr {
    udp_port_t udp_sport;   /*!< UDP Source port. */
    udp_port_t udp_dport;   /*!< UDP Destination port. */
    uint16_t udp_len;       /*!< UDP datagram length. */
    uint16_t udp_csum;      /*!< UDP Checksum. */
    uint8_t data[0];        /*!< Datagram contents. */
};

struct xstack_sock;
struct xstack_dgram;

int xstack_udp_bind(struct xstack_sock * sock);
int xstack_udp_send(struct xstack_sock * sock,
                    const struct xstack_dgram * dgram);

#endif /* XSTACK_UDP_H */

/**
 * @}
 */
