#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xstack.h"
#include "xstack_arp.h"
#include "xstack_icmp.h"
#include "xstack_in.h"
#include "xstack_ip.h"
#include "xstack_socket.h"

#include "ip_defer.h"
#include "logger.h"
#include "tree.h"
#include "udp.h"
#include "xstack_internal.h"

struct udp_socket {
    struct xstack_sockaddr addr;
    struct xstack_sock sock;
    RB_ENTRY(udp_socket) _entry;
};

struct udp_socket_find {
    struct xstack_sockaddr addr;
};

static RB_HEAD(udp_socket_tree, udp_socket) upd_socket_tree_head =
    RB_INITIALIZER(_head);

static int udp_socket_cmp(struct udp_socket * a, struct udp_socket * b)
{
    return memcmp(&a->addr, &b->addr, sizeof(struct xstack_sockaddr));
}

RB_GENERATE_STATIC(udp_socket_tree, udp_socket, _entry, udp_socket_cmp);

static struct udp_socket * find_udp_socket(const struct xstack_sockaddr * addr)
{
    struct udp_socket_find find = {
        .addr = *addr,
    };

    return RB_FIND(udp_socket_tree, &upd_socket_tree_head,
                   (struct udp_socket *)(&find));
}

static void udp_hton(const struct udp_hdr * host, struct udp_hdr * net)
{
    net->udp_sport = htons(host->udp_sport);
    net->udp_dport = htons(host->udp_dport);
    net->udp_len = htons(host->udp_len);
}

static void udp_ntoh(const struct udp_hdr * net, struct udp_hdr * host)
{
    host->udp_sport = ntohs(net->udp_sport);
    host->udp_dport = ntohs(net->udp_dport);
    host->udp_len = ntohs(net->udp_len);
}

struct xstack_sock * xstack_udp_alloc_sock(void)
{
    struct udp_socket * udp_sock;

    udp_sock = calloc(1, sizeof(struct udp_socket));
    return (udp_sock) ? &udp_sock->sock : NULL;
}

int xstack_udp_bind(struct xstack_sock * sock,
                    const struct xstack_sockaddr * sockaddr)
{
    struct udp_socket * udp_sock = container_of(sock, struct udp_socket, sock);

    if (sockaddr->port > XSTACK_SOCK_PORT_MAX) {
        errno = EINVAL;
        return -1;
    }

    if (find_udp_socket(sockaddr)) {
        errno = EADDRINUSE;
        return -1;
    }

    udp_sock->addr = *sockaddr;
    RB_INSERT(udp_socket_tree, &upd_socket_tree_head, udp_sock);

    return 0;
}

/**
 * UDP input chain.
 * IP -> UDP
 */
static int udp_input(const struct ip_hdr * ip_hdr, uint8_t * payload, size_t bsize)
{
    struct udp_hdr * udp = (struct udp_hdr *)payload;
    struct udp_socket * udp_sock;
    struct xstack_sockaddr sockaddr;

    if (bsize < sizeof(struct udp_hdr)) {
        LOG(LOG_INFO, "Datagram size too small");

        return -EBADMSG;
    }

    udp_ntoh(udp, udp);

    sockaddr.inet4_addr = ip_hdr->ip_dst;
    sockaddr.port = udp->udp_dport;
    udp_sock = find_udp_socket(&sockaddr);
    if (udp_sock) {
        int retval;
        struct xstack_sockaddr srcaddr = {
            .inet4_addr = ip_hdr->ip_src,
            .port = udp->udp_sport,
        };

        retval = xstack_sock_dgram_input(&udp_sock->sock,
                                         &srcaddr,
                                         payload + sizeof(struct udp_hdr),
                                         bsize - sizeof(struct udp_hdr));
        if (retval > 0) {
            /*
             * RFE The following code is probably not needed as
             * xstack_sock_dgram_input() never returns anything above 0.
             */
            udp_port_t tmp;

            /* Swap ports */
            tmp = udp->udp_sport;
            udp->udp_sport = udp->udp_dport;
            udp->udp_dport = tmp;

            udp->udp_len = sizeof(struct udp_hdr) + retval;

            if (IP_VERSION(ip_hdr) == 4) {
                udp->udp_csum = 0; /* Can be zeroed on IPv4 */
            } else {
                /* TODO update csum */
                udp->udp_csum = 0;
            }

            udp_hton(udp, udp);
        }
        return retval;
    } else {
        LOG(LOG_INFO, "Port %d unreachable", sockaddr.port);

        return -ENOTSOCK;
    }
}
IP_PROTO_INPUT_HANDLER(IP_PROTO_UDP, udp_input);

int xstack_udp_send(int fd, const struct xstack_send_args * args)
{
    uint8_t buf[sizeof(struct udp_hdr) + args->buf_size];
    struct udp_hdr * udp = (struct udp_hdr *)buf;
    uint8_t * payload = udp->data;
    struct udp_socket * udp_sock = container_of(args->sock, struct udp_socket,
                                                sock);

    /*
     * UDP Header.
     */
    udp->udp_sport = udp_sock->addr.port;
    udp->udp_dport = args->dstaddr.port;
    udp->udp_len = sizeof(struct udp_hdr) + args->buf_size;
    udp->udp_csum = 0; /* TODO calc UDP csum */

    /*
     * Read the payload from the egress pipe.
     */
    read(fd, payload, args->buf_size);

    udp_hton(udp, udp);
    return ip_send(args->dstaddr.inet4_addr, IP_PROTO_UDP, buf, sizeof(buf));
}
