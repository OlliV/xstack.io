#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "xstack_arp.h"
#include "xstack_icmp.h"
#include "xstack_in.h"
#include "xstack_ip.h"
#include "xstack_udp.h"

#include "ip_defer.h"
#include "logger.h"

SET_DECLARE(_udp_socket_handlers, struct _udp_socket_handler);

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

static int udp_input(const struct ip_hdr * ip_hdr, uint8_t * payload, size_t bsize)
{
    struct udp_hdr * udp = (struct udp_hdr *)payload;
    struct _udp_socket_handler ** tmpp;
    struct _udp_socket_handler * socket;

    if (bsize < sizeof(struct udp_hdr)) {
        LOG(LOG_INFO, "Packet size too small");

        return -EBADMSG;
    }

    udp_ntoh(udp, udp);

    SET_FOREACH(tmpp, _udp_socket_handlers) {
        socket = *tmpp;
        if (socket->port == udp->udp_dport) {
            break;
        }
        socket = NULL;
    }

    if (socket) {
        struct udp_com_hdr com = {
            .src_ip = ip_hdr->ip_src,
            .dst_ip = ip_hdr->ip_dst,
            .udp = udp,
        };
        int retval;

        retval = socket->fn(&com, payload + sizeof(struct udp_hdr),
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

static int udp_port0(const struct udp_com_hdr * hdr __unused,
                     uint8_t * payload __unused, size_t bsize __unused)
{
    return -ENOTSOCK;
}
UDP_PROTO_SOCKET(0, udp_port0);
