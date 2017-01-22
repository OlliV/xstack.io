/**
 * Xstack sockets.
 * @addtogroup Socket
 * @{
 */

#ifndef XSTACK_SOCKET_H
#define XSTACK_SOCKET_H

#include <stdint.h>
#include <sys/types.h>

#include "linker_set.h"
#include "queue_r.h"
#include "xstack_in.h"

#define XSTACK_SHMEM_SIZE \
    (sizeof(struct xstack_sock_ctrl) + \
     2 * sizeof(struct queue_cb) + \
     2 * XSTACK_DATAGRAM_BUF_SIZE)

#define XSTACK_SOCK_CTRL(x) \
    ((struct xstack_sock_ctrl *)(x))

#define XSTACK_INGRESS_QADDR(x) \
    ((struct queue_cb *)((uintptr_t)XSTACK_SOCK_CTRL(x) + \
                         sizeof(struct xstack_sock_ctrl)))

#define XSTACK_INGRESS_DADDR(x) \
    ((uint8_t *)((uintptr_t)XSTACK_INGRESS_QADDR(x) + \
                 sizeof(struct queue_cb)))

#define XSTACK_EGRESS_QADDR(x) \
    ((struct queue_cb *)((uintptr_t)XSTACK_INGRESS_DADDR(x) + \
                         XSTACK_DATAGRAM_BUF_SIZE))

#define XSTACK_EGRESS_DADDR(x) \
    ((uint8_t *)((uintptr_t)XSTACK_EGRESS_QADDR(x) + \
                 sizeof(struct queue_cb)))

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
    XIP_PROTO_NONE = 0,
    XIP_PROTO_TCP, /*!< TCP/IP. */
    XIP_PROTO_UDP, /*!< UDP/IP. */
    XIP_PROTO_LAST
};

/**
 * Max port number.
 */
#define XSTACK_SOCK_PORT_MAX 49151

/**
 * Socket addresss descriptor.
 */
struct xstack_sockaddr {
    union {
        in_addr_t inet4_addr; /*!< IPv4 address. */
    };
    union {
        int port; /*!< Protocol port. */
    };
};

struct xstack_sock_ctrl {
    pid_t pid_inetd;
    pid_t pid_end;
};

struct xstack_sock_info {
    enum xstack_sock_dom sock_dom;
    enum xstack_sock_type sock_type;
    enum xstack_sock_proto sock_proto;
    struct xstack_sockaddr sock_addr;
} info;

struct xstack_dgram {
    struct xstack_sockaddr srcaddr;
    struct xstack_sockaddr dstaddr;
    size_t buf_size;
    uint8_t buf[0];
};

#define XSTACK_MSG_PEEK 0x1

void * xstack_listen(const char * socket_path);
ssize_t xstack_recvfrom(void * socket, void * restrict buffer, size_t length,
                        int flags, struct xstack_sockaddr * restrict address);
ssize_t xstack_sendto(void * socket, const void * buffer, size_t length,
                      int flags, const struct xstack_sockaddr * dest_addr);

#endif /* XSTACK_SOCKET_H */

/**
 * @}
 */
