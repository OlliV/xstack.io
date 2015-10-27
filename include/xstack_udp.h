/**
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

/**
 * UDP socket header.
 */
struct udp_com_hdr {
    in_addr_t src_ip;       /*!< Source IPv4 address. */
    in_addr_t dst_ip;       /*!< Destination IPv4 address. */
    struct udp_hdr * udp;   /*!< Pointer to the UDP header. */
};

struct _udp_socket_handler {
    uint16_t port;
    int (*fn)(const struct udp_com_hdr * hdr, uint8_t * payload, size_t bsize);
};

/**
 * Declare a static UDP socket.
 */
#define UDP_PROTO_SOCKET(_port_, _handler_fn_)                               \
    static struct _udp_socket_handler _udp_socket_handler_##_handler_fn_ = { \
        .port = _port_,                                                      \
        .fn = _handler_fn_,                                                  \
    };                                                                       \
    DATA_SET(_udp_socket_handlers, _udp_socket_handler_##_handler_fn_)

#endif /* XSTACK_UDP_H */

/**
 * @}
 */
