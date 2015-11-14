/**
 * @addtogroup Socket
 * @{
 */

#ifndef XSTACK_SOCKET_H
#define XSTACK_SOCKET_H

#include <stdint.h>

#include "linker_set.h"
#include "xstack_in.h"

/**
 * Socket domain.
 */
enum xstack_sock_dom {
    XF_INET4, /*!< IPv4 address. */
    XF_INET6, /*!< IPv6 address. */
};

/**
 * Socket type.
 */
enum xstack_sock_type {
    XSOCK_DGRAM, /*!< Unreliable datagram oriented service. */
    XSOCK_STREAM, /*!< Reliable stream oriented service. */
};

/**
 * Socket protocol.
 */
enum xstack_sock_proto {
    XIP_PROTO_TCP, /*!< TCP/IP. */
    XIP_PROTO_UDP, /*!< UDP/IP. */
};

/**
 * A generic socket descriptor.
 */
struct xstack_sock {
    enum xstack_sock_dom sock_dom;
    enum xstack_sock_type sock_type;
    enum xstack_sock_proto sock_proto;
    int ingress[2];
    int egress[2];
};

/**
 * Socket addresss descriptor.
 */
struct xstack_sockaddr {
    union {
        in_addr_t inet4_addr;
    };
    union {
        int port;
    };
};

/**
 * Create a endpoint socket for communication.
 * @param dom selects the socket domain.
 * @param type selects the socket type.
 * @param proto selects the protocol used.
 * @returns Uppon succesful completion returns a pointer to the socket;
 *          Otherwise a NULL pointer is returned.
 */
struct xstack_sock * xstack_socket(enum xstack_sock_dom dom,
                                   enum xstack_sock_type type,
                                   enum xstack_sock_proto proto);

/**
 * Bind an address to a socket.
 * @param[in] sock is a pointer to the socket returned by xstack_socket().
 * @param sockaddr is the address.
 */
int xstack_bind(struct xstack_sock * sock, struct xstack_sockaddr sockaddr);

/**
 * Start listening on a stream socket.
 * @param[in] sock is a pointer to the socket returned by xstack_socket().
 * @param backlog is maximum number of pending connections.
 * @returns Uppon succesful completion returns 0;
 *          Otherwise -1 is returned and errno is set.
 */
int xstack_listen(struct xstack_sock * sock, int backlog);

/**
 * Accept a new connection on a socket.
 * param[in] sock is a pointer to the socket returned by xstack_socket().
 * @param[out] cliaddr is used to return the client address.
 * @returns Returns a pointer to a xstack_sock struct describing the accepted
 *          socket.
 */
struct xstack_sock * xstack_accept(struct xstack_sock * sock,
                                   struct xstack_sockaddr * cliaddr);

/**
 * Receive from a socket.
 * @param[in] sock is a pointer to the socket returned by xstack_socket().
 * @param buf is a pointer to the buffer.
 * @param bsize is the size of the buffer.
 * @param flags contains the optional flags.
 * @param[out] srcaddr returns the source address.
 */
int xstack_recvfrom(struct xstack_sock * sock, void * buf, size_t bsize,
                    unsigned flags, struct xstack_sockaddr * srcaddr);

/**
 * Send to a socket.
 * @param sock[in] sock is a pointer to the socket returned by xstack_socket().
 * @param buf is a pointer to the buffer to be transmitted.
 * @param bsize is the size of the buffer.
 * @param flags contains the optional flags.
 */
int xstack_send(struct xstack_sock * sock, void * buf, size_t bsize,
                unsigned flags);

int _xstack_sock_dgram_input(struct xstack_sock * sock,
                             struct xstack_sockaddr * srcaddr,
                             uint8_t * buf, size_t bsize);

#endif /* XSTACK_SOCKET_H */

/**
 * @}
 */
