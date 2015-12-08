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

struct xstack_sockaddr;

int xstack_udp_bind(struct xstack_sock * sock,
                    const struct xstack_sockaddr * sockaddr);

#endif /* XSTACK_UDP_H */

/**
 * @}
 */
