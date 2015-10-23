#include <string.h>
#include "xstack_icmp.h"
#include "xstack_ip.h"
#include "logger.h"

static void icmp_hton(struct icmp * host, struct icmp * net)
{
    net->icmp_type  = host->icmp_type;
    net->icmp_code  = host->icmp_code;
    net->icmp_csum  = host->icmp_csum;
    net->icmp_id    = htons(host->icmp_id);
    net->icmp_seqno = htons(host->icmp_seqno);
}

static void icmp_ntoh(struct icmp * net, struct icmp * host)
{
    host->icmp_type  = net->icmp_type;
    host->icmp_code  = net->icmp_code;
    host->icmp_csum  = net->icmp_csum;
    host->icmp_id    = htons(net->icmp_id);
    host->icmp_seqno = htons(net->icmp_seqno);
}

void icmp_input(const struct ip_hdr * ip_hdr, uint8_t * payload, size_t bsize)
{
    struct icmp * net_msg = (struct icmp *)payload;
    struct icmp hdr;
    size_t msg_size;

    icmp_ntoh(net_msg, &hdr);
    msg_size = ip_hdr->ip_len - ip_hdr_hlen(ip_hdr);

    LOG(LOG_DEBUG, "ICMP type: %d", hdr.icmp_type);
    switch (hdr.icmp_type) {
    case ICMP_TYPE_ECHO:
        net_msg->icmp_type = ICMP_TYPE_ECHO_REPLY;
        net_msg->icmp_csum = 0;
        net_msg->icmp_csum = ip_checksum(net_msg, msg_size);

        ip_send(ip_hdr->ip_dst, ip_hdr->ip_src, IP_PROTO_ICMP, payload, msg_size);
        break;
    default:
        LOG(LOG_WARN, "Unkown ICMP message type");
        return;
    }
}
IP_PROTO_INPUT_HANDLER(IP_PROTO_ICMP, icmp_input);
