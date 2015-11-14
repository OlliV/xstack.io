#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xstack_in.h"
#include "xstack_ip.h"
#include "xstack_socket.h"
#include "xstack_udp.h"

#include "logger.h"
#include "queue.h"

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

    sock = malloc(sizeof(struct xstack_sock));
    if (!sock) {
        errno = ENOMEM;
        return NULL;
    }

    if (pipe2(sock->ingress, pipe2_flags) ||
        pipe2(sock->egress, pipe2_flags)) {
        free(sock);
        return NULL;
    }

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
    errno = ENOTSUP;
    return -1;
}

struct xstack_sock * xstack_accept(struct xstack_sock * sock,
                                   struct xstack_sockaddr * cliaddr)
{
    return NULL;
}

int xstack_recvfrom(struct xstack_sock * sock, void * buf, size_t bsize,
                    unsigned flags, struct xstack_sockaddr * srcaddr)
{
    int fd = sock->ingress[0];
    struct xstack_sockaddr src;

    read(fd, &src, sizeof(struct xstack_sockaddr));
    if (srcaddr) {
        *srcaddr = src;
    }
    return (int)read(fd, buf, bsize);
}

int xstack_send(struct xstack_sock * sock, void * buf, size_t bsize,
                unsigned flags)
{
    int fd = sock->egress[1];

    /* TODO write header */
    return (int)write(fd, buf, bsize);
}

int _xstack_sock_dgram_input(struct xstack_sock * sock,
                             struct xstack_sockaddr * srcaddr,
                             uint8_t * buf, size_t bsize)
{
    int fd = sock->ingress[1];

    write(fd, srcaddr, sizeof(struct xstack_sockaddr));
    write(fd, buf, bsize);

    return 0;
}
