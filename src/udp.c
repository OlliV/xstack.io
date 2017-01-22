#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xstack_in.h"
#include "xstack_socket.h"

#include "ip_defer.h"
#include "logger.h"
#include "udp.h"
#include "xstack_arp.h"
#include "xstack_icmp.h"
#include "xstack_internal.h"
#include "xstack_ip.h"

static RB_HEAD(udp_sock_tree, xstack_sock) upd_sock_tree_head =
    RB_INITIALIZER(_head);

static int udp_socket_cmp(struct xstack_sock * a, struct xstack_sock * b)
{
    return memcmp(&a->info.sock_addr, &b->info.sock_addr, sizeof(struct xstack_sockaddr));
}

RB_GENERATE_STATIC(udp_sock_tree, xstack_sock, data.udp._entry, udp_socket_cmp);

static struct xstack_sock * find_udp_socket(const struct xstack_sockaddr * addr)
{
    struct xstack_sock_info find = {
        .sock_addr = *addr,
    };

    return RB_FIND(udp_sock_tree, &upd_sock_tree_head,
                   (struct xstack_sock *)(&find));
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

int xstack_udp_bind(struct xstack_sock * sock)
{
    if (sock->info.sock_addr.port > XSTACK_SOCK_PORT_MAX) {
        errno = EINVAL;
        return -1;
    }

    if (find_udp_socket(&sock->info.sock_addr)) {
        errno = EADDRINUSE;
        return -1;
    }

    RB_INSERT(udp_sock_tree, &upd_sock_tree_head, sock);

    return 0;
}

/**
 * UDP input chain.
 * IP -> UDP
 */
static int udp_input(const struct ip_hdr * ip_hdr, uint8_t * payload, size_t bsize)
{
    struct udp_hdr * udp = (struct udp_hdr *)payload;
    struct xstack_sock * sock;
    struct xstack_sockaddr sockaddr;

    if (bsize < sizeof(struct udp_hdr)) {
        LOG(LOG_INFO, "Datagram size too small");

        return -EBADMSG;
    }

    udp_ntoh(udp, udp);

    sockaddr.inet4_addr = ip_hdr->ip_dst;
    sockaddr.port = udp->udp_dport;
    sock = find_udp_socket(&sockaddr);
    if (sock) {
        int retval;
        struct xstack_sockaddr srcaddr = {
            .inet4_addr = ip_hdr->ip_src,
            .port = udp->udp_sport,
        };

        retval = xstack_sock_dgram_input(sock,
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

int xstack_udp_send(struct xstack_sock * sock,
                    const struct xstack_dgram * dgram)
{
    uint8_t buf[sizeof(struct udp_hdr) + dgram->buf_size];
    struct udp_hdr * udp = (struct udp_hdr *)buf;
    uint8_t * payload = udp->data;

    if (!(dgram->buf_size > 0 && dgram->buf_size < UDP_MAXLEN)) {
        return -EINVAL;
    }

    /*
     * UDP Header.
     */
    udp->udp_sport = sock->info.sock_addr.port;
    udp->udp_dport = dgram->dstaddr.port;
    udp->udp_len = sizeof(struct udp_hdr) + dgram->buf_size;
    udp->udp_csum = 0; /* TODO calc UDP csum */

    memcpy(payload, dgram->buf, dgram->buf_size);

    udp_hton(udp, udp);
    return ip_send(dgram->dstaddr.inet4_addr, IP_PROTO_UDP, buf, sizeof(buf));
}
