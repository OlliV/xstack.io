#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xstack_arp.h"
#include "xstack_icmp.h"
#include "xstack_in.h"
#include "xstack_ip.h"
#include "xstack_socket.h"
#include "xstack_udp.h"

#include "ip_defer.h"
#include "logger.h"
#include "tree.h"

#define UDP_PORT_MAX 100

struct udp_socket {
    in_addr_t inet4_addr;
    struct xstack_sock * portmap[UDP_PORT_MAX];
    RB_ENTRY(udp_socket) _entry;
};

struct udp_socket_find {
    in_addr_t inet4_addr;
};

static RB_HEAD(udp_socket_tree, udp_socket) upd_socket_tree_head =
    RB_INITIALIZER(_head);

static int udp_socket_cmp(struct udp_socket * a, struct udp_socket * b)
{
    return a->inet4_addr - b->inet4_addr;
}

RB_GENERATE_STATIC(udp_socket_tree, udp_socket, _entry, udp_socket_cmp);

static struct udp_socket * find_udp_socket(in_addr_t addr)
{
    struct udp_socket_find find = {
        .inet4_addr = addr,
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

int xstack_udp_bind(struct xstack_sock * sock,
                    const struct xstack_sockaddr * sockaddr)
{
    int port = sockaddr->port;
    struct udp_socket * udp_sock;

    if (sockaddr->port >= UDP_PORT_MAX) {
        errno = EINVAL;
        return -1;
    }

    udp_sock = find_udp_socket(sockaddr->inet4_addr);
    if (!udp_sock) {
        udp_sock = calloc(1, sizeof(struct udp_socket));
        if (!udp_sock) {
            errno = ENOMEM;
            return -1;
        }
        udp_sock->inet4_addr = sockaddr->inet4_addr;
        RB_INSERT(udp_socket_tree, &upd_socket_tree_head, udp_sock);
    }

    if (udp_sock->portmap[port]) {
        errno = EADDRINUSE;
        return -1;
    } else {
        udp_sock->portmap[port] = sock;
        return 0;
    }
}

static int udp_input(const struct ip_hdr * ip_hdr, uint8_t * payload, size_t bsize)
{
    struct udp_hdr * udp = (struct udp_hdr *)payload;
    struct udp_socket * udp_sock;

    if (bsize < sizeof(struct udp_hdr)) {
        LOG(LOG_INFO, "Datagram size too small");

        return -EBADMSG;
    }

    udp_ntoh(udp, udp);

    udp_sock = find_udp_socket(ip_hdr->ip_dst);
    if (udp_sock && udp->udp_dport < UDP_PORT_MAX &&
        udp_sock->portmap[udp->udp_dport]) {
        int retval;
        struct xstack_sockaddr srcaddr = {
            .inet4_addr = ip_hdr->ip_src,
            .port = udp->udp_sport,
        };

        retval =_xstack_sock_dgram_input(udp_sock->portmap[udp->udp_dport],
                                         &srcaddr,
                                         payload + sizeof(struct udp_hdr),
                                         bsize - sizeof(struct udp_hdr));
        if (retval > 0) {
            udp_port_t tmp;

            /* Swap ports */
            tmp = udp->udp_sport;
            udp->udp_sport = udp->udp_dport;
            udp->udp_dport = tmp;

            udp->udp_len = retval;

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
        LOG(LOG_INFO, "Port unreachable");

        return -ENOTSOCK;
    }
}
IP_PROTO_INPUT_HANDLER(IP_PROTO_UDP, udp_input);
