#include <errno.h>
#include <string.h>

#include "xstack_ip.h"

#include "logger.h"
#include "xstack_internal.h"
#include "tcp.h"

static void udp_hton(const struct udp_hdr * host, struct udp_hdr * net)
{
    net->tcp_sport = htons(host->tcp_sport);
    net->tcp_dport = htons(host->tcp_dport);
    net->tcp_seqno = htonl(host->tcp_seqno);
    net->tcp_ack_num = htonl(host->tcp_ack_num);
    net->tcp_win_size = htons(host->tcp_win_size);
    net->tcp_checksum = htons(host->tcp_checksum);
    net->tcp_urg_ptr = htons(host->tcp_urg_ptr);
}

static void tcp_ntoh(const struct tcp_hdr * net, struct tcp_hdr * host)
{
    host->tcp_sport = ntohs(net->tcp_sport);
    host->tcp_dport = ntohs(net->tcp_dport);
    host->tcp_seqno = ntohl(net->tcp_seqno);
    host->tcp_ack_num = ntohl(net->tcp_ack_num);
    host->tcp_win_size = ntohs(net->tcp_win_size);
    host->tcp_checksum = ntohs(net->tcp_checksum);
    host->tcp_urg_ptr = ntohs(net->tcp_urg_ptr);
}

/**
 * TCP input chain.
 * IP -> TCP
 */
static int tcp_input(const struct ip_hdr * ip_hdr, uint8_t * payload, size_t bsize)
{
    struct tcp_hdr * tcp = (struct tcp_hdr *)payload;

    if (bsize < sizeof(struct tcp_hdr)) {
        LOG(LOG_INFO, "Datagram size too small");

        return -EBADMSG;
    }

    tcp_ntoh(tcp, tcp);

    return -1;
}
IP_PROTO_INPUT_HANDLER(IP_PROTO_TCP, tcp_input);

int xstack_tcp_send(struct xstack_sock * sock,
                    const struct xstack_dgram * dgram)
{
    return -1;
}
