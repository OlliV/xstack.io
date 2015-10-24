#include <errno.h>
#include <string.h>

#include "xstack_icmp.h"
#include "xstack_ip.h"

#include "logger.h"

static void icmp_hton(struct icmp * host, struct icmp * net)
{
    net->icmp_type  = host->icmp_type;
    net->icmp_code  = host->icmp_code;
    net->icmp_csum  = host->icmp_csum;
}

static void icmp_ntoh(struct icmp * net, struct icmp * host)
{
    host->icmp_type  = net->icmp_type;
    host->icmp_code  = net->icmp_code;
    host->icmp_csum  = net->icmp_csum;
}

static int icmp_input(const struct ip_hdr * ip_hdr, uint8_t * payload, size_t bsize)
{
    struct icmp * net_msg = (struct icmp *)payload;
    struct icmp hdr;
    size_t msg_size;

    if (bsize < sizeof(struct icmp)) {
        LOG(LOG_ERR, "Invalid ICMP message size");

        errno = EBADMSG;
        return -1;
    }

    icmp_ntoh(net_msg, &hdr);
    msg_size = ip_hdr->ip_len - ip_hdr_hlen(ip_hdr);

    LOG(LOG_DEBUG, "ICMP type: %d", hdr.icmp_type);
    switch (hdr.icmp_type) {
    case ICMP_TYPE_ECHO:
        net_msg->icmp_type = ICMP_TYPE_ECHO_REPLY;
        net_msg->icmp_csum = 0;
        net_msg->icmp_csum = ip_checksum(net_msg, msg_size);

        return msg_size;
    default:
        LOG(LOG_INFO, "Unkown ICMP message type");

        errno = ENOMSG;
        return -1;
    }
}
IP_PROTO_INPUT_HANDLER(IP_PROTO_ICMP, icmp_input);
