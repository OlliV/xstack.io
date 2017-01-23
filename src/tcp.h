/**
 * Xstack TCP service.
 * @addtogroup TCP
 * @{
 */

#ifndef XSTACK_TCP_H
#define XSTACK_TCP_H

#include <stdint.h>

#include "linker_set.h"
#include "xstack_in.h"

/**
 * Type for an TCP port number.
 */
typedef uint16_t tcp_port_t;

/**
 * TCP packet header.
 */
struct tcp_hdr {
    struct {
        unsigned tcp_sport      : 16;   /*!< Source port. */
        unsigned tcp_dport      : 16;   /*!< Destination port. */
        unsigned tcp_seqno      : 32;   /*!< Sequence number.*/
        unsigned tcp_ack_num    : 32;   /*!< Acknowledgment number (if ACK). */
        unsigned tcp_data_off   : 4;    /*<! Data offset. */
        unsigned _reserved      : 3;
        unsigned tcp_ns         : 1;    /*!< ECN-nonce concealment prot. */
        unsigned tcp_cwr        : 1;    /*!< Congestion Window Reduced. */
        unsigned tcp_ece        : 1;    /*!< ECN-Echo. */
        unsigned tcp_urg        : 1;    /*!< Check Urgent pointer. */
        unsigned tcp_ack        : 1;    /*!< Check ack_num. */
        unsigned tcp_psh        : 1;    /*!< Push function. */
        unsigned tcp_rst        : 1;    /*!< Reset the connection. */
        unsigned tcp_syn        : 1;    /*!< Synchronize sequence numbers. */
        unsigned tcp_fin        : 1;    /*!< Last package from sender. */
        unsigned tcp_win_size   : 16;   /*!< Window size. */
        unsigned tcp_checksum   : 16;   /*!< TCP Checksum. */
        unsigned tcp_urg_ptr    : 16;   /*!< Urgent pointer (if URG). */
    };
    uint8_t opt[0];                      /*!< Options. */
} __attribute__((packed, aligned(4)));

struct xstack_sockaddr;

/**
 * Allocate a UDP socket descriptor.
 */
struct xstack_sock * xstack_udp_alloc_sock(void);

int xstack_tcp_bind(struct xstack_sock * sock);
int xstack_tcp_send(struct xstack_sock * sock,
                    const struct xstack_dgram * dgram);

#endif /* XSTACK_UDP_H */

/**
 * @}
 */
