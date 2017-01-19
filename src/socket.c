#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xstack.h"
#include "xstack_in.h"
#include "xstack_ip.h"
#include "xstack_socket.h"

#include "logger.h"
#include "queue.h"
#include "udp.h"
#include "xstack_internal.h"

struct xstack_sock * xstack_socket(enum xstack_sock_dom dom,
                                   enum xstack_sock_type type,
                                   enum xstack_sock_proto proto)
{
    struct xstack_sock * sock;
    int pipe2_flags;


    switch (type) {
    case XSOCK_DGRAM:
        pipe2_flags = O_DIRECT;
        break;
    case XSOCK_STREAM:
        pipe2_flags = 0;
        break;
    default:
        errno = ENOTSUP;
        return NULL;
    }

    switch (proto) {
    case XIP_PROTO_UDP:
        sock = xstack_udp_alloc_sock();
        if (!sock) {
            errno = ENOMEM;
            return NULL;
        }
        break;
    default:
        errno = EPROTOTYPE;
        return NULL;
    }

    if (pipe2(sock->ingress, pipe2_flags) ||
        pipe2(sock->egress, pipe2_flags)) {
        free(sock);
        return NULL;
    }

    xstack_egress_add_fd(sock->egress[0]);

    sock->sock_dom = dom;
    sock->sock_type = type;
    sock->sock_proto = proto;

    return sock;
}

int xstack_bind(struct xstack_sock * sock, struct xstack_sockaddr sockaddr)
{
    switch (sock->sock_proto) {
    case XIP_PROTO_UDP:
        return xstack_udp_bind(sock, &sockaddr);
    default:
        errno = EPROTOTYPE;
        return -1;
    }
}

int xstack_listen(struct xstack_sock * sock, int backlog)
{
    /* TODO */
    errno = ENOTSUP;
    return -1;
}

struct xstack_sock * xstack_accept(struct xstack_sock * sock,
                                   struct xstack_sockaddr * cliaddr)
{
    /* TODO */
    return NULL;
}

int xstack_recvfrom(struct xstack_sock * sock, void * buf, size_t bsize,
                    unsigned flags, struct xstack_sockaddr * srcaddr)
{
    int fd = sock->ingress[0];
    struct xstack_cmsg_dgram_input ctrl_msg;

    read(fd, &ctrl_msg, sizeof(ctrl_msg));
    if (srcaddr) {
        *srcaddr = ctrl_msg.srcaddr;
    }
    return (int)read(fd, buf, bsize);
}

int xstack_send(struct xstack_sock * sock, void * buf, size_t bsize,
                const struct xstack_sockaddr * dstaddr, unsigned flags)
{
    int fd = sock->egress[1];
    struct xstack_cmsg_dgram_send args = {
        .ctrl.ctrl_flags = 0,
        .sock = sock,
        .buf_size = bsize,
        .flags = flags,
    };

    /*
     * dstaddr is only necessary for stateless protocols.
     */
    if (dstaddr) {
        args.dstaddr = *dstaddr;
    }

    /*
     * Datagrams in egress chain are delivered as two packets, first comes
     * the descriptor and the second packet is the actual datagram contents.
     */
    write(fd, &args, sizeof(args));
    return (int)write(fd, buf, bsize);
}

void xstack_close(struct xstack_sock * sock)
{
    int * i = &sock->ingress[0];
    int * e = &sock->egress[1];

    /* TODO Send control message to close the reading end in egress */

    close(*i);
    close(*e);

    /*
     * Mark file descriptors closed.
     */
    *i = -1;
    *e = -1;
}

int xstack_sock_dgram_input(struct xstack_sock * sock,
                            struct xstack_sockaddr * srcaddr,
                            uint8_t * buf, size_t bsize)
{
    int fd = sock->ingress[1];
    struct xstack_cmsg_dgram_input ctrl_msg = {
        .ctrl.ctrl_flags = 0,
        .srcaddr = *srcaddr,
    };

    /*
     * Datagrams in ingress chain are delivered as two packets, first comes
     * the control message and the second packet is the actual datagram
     * contents.
     */
    if (write(fd, &ctrl_msg, sizeof(ctrl_msg)) == -1) {
        return -errno;
    }
    return (write(fd, buf, bsize) != -1) ? 0 : -errno;
}
