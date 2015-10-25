#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "xstack_arp.h"
#include "xstack_ether.h"
#include "xstack_in.h"
#include "xstack_ip.h"

#include "ip_defer.h"
#include "logger.h"

SET_DECLARE(_ip_proto_handlers, struct _ip_proto_handler);
SET_DECLARE(_ip_periodic_tasks, void);

int ip_config(int ether_handle, in_addr_t ip_addr, in_addr_t netmask)
{
    mac_addr_t mac;
    struct ip_route route = {
        .r_network = ip_addr & netmask,
        .r_netmask = netmask,
        .r_gw = 0, /* TODO GW support */
        .r_iface = ip_addr,
        .r_iface_handle = ether_handle,
    };

    ether_handle2addr(ether_handle, mac);
    arp_cache_insert(ip_addr, mac, ARP_CACHE_STATIC);

    ip_route_update(&route);

    /* Announce that we are online. */
    for (size_t i = 0; i < 3; i++) {
        arp_gratuitous(ether_handle, ip_addr);
    }

    return 0;
}

void ip2str(in_addr_t ip, char * buf)
{
    unsigned char bytes[4];

    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
    sprintf(buf, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
}

uint16_t ip_checksum(void * dp, size_t bsize)
{
    uint8_t * data = (uint8_t *)dp;
    uint32_t acc = 0xffff;
    size_t i;
    uint16_t word;

    for (i = 0; i + 1 < bsize; i += 2) {

        memcpy(&word, data + i, 2);
        acc += word;
        if (acc > 0xffff) {
            acc -= ntohs(0xffff);
        }
    }

    if (bsize & 1) {
        word = 0;
        memcpy(&word, data + bsize - 1, 1);
        acc += word;
        if (acc > 0xffff) {
            acc -= ntohs(0xffff);
        }
    }

    return ~acc;
}

void ip_run_periodic_tasks(int delta_time)
{
    void ** taskp;

    SET_FOREACH(taskp, _ip_periodic_tasks) {
        ip_periodic_task_t * task = *(ip_periodic_task_t **)taskp;

        if (task)
            task(delta_time);
    }
}

static void ip_hton(struct ip_hdr * host, struct ip_hdr * net)
{
    net->ip_vhl = host->ip_vhl;
    net->ip_tos = host->ip_tos;
    net->ip_len = htons(host->ip_len);
    net->ip_id = htons(host->ip_id);
    net->ip_foff = htons(host->ip_foff);
    net->ip_ttl = host->ip_ttl;
    net->ip_proto = host->ip_proto;
    net->ip_csum = host->ip_csum;
    net->ip_src = htonl(host->ip_src);
    net->ip_dst = htonl(host->ip_dst);
}

static void ip_ntoh(struct ip_hdr * net, struct ip_hdr * host)
{
    host->ip_vhl = net->ip_vhl;
    host->ip_tos = net->ip_tos;
    host->ip_len = ntohs(net->ip_len);
    host->ip_id = ntohs(net->ip_id);
    host->ip_foff = ntohs(net->ip_foff);
    host->ip_ttl = net->ip_ttl;
    host->ip_proto = net->ip_proto;
    host->ip_csum = net->ip_csum;
    host->ip_src = ntohl(net->ip_src);
    host->ip_dst = ntohl(net->ip_dst);
}

static int ip_input(const struct ether_hdr * hdr, uint8_t * payload, size_t bsize)
{
    struct ip_hdr * ip = (struct ip_hdr *)payload;
    struct _ip_proto_handler ** tmpp;
    struct _ip_proto_handler * proto;
    size_t hlen;

    ip_ntoh(ip, ip);

    if ((ip->ip_vhl & 0x40) != 0x40) {
        LOG(LOG_ERR, "Unsupported IP packet version: 0x%x", ip->ip_vhl);
        return 0;
    }

    hlen = ip_hdr_hlen(ip);
    if (hlen < 20 || ip->ip_len > bsize) {
        LOG(LOG_ERR, "Incorrect packet header length: %d", (int)hlen);
        return 0;
    }

    /*
     * RFE: The packet header is already modified with ntoh so this wont work.
     */
#if 0
    if (ip_checksum(ip, hlen) != 0) {
        LOG(LOG_ERR, "Drop due to an invalid checksum");
        return;
    }
#endif

    if (ip->ip_tos != IP_TOS_DEFAULT) {
        LOG(LOG_ERR, "Unsupported IP type of service or ECN: 0x%x", ip->ip_tos);
        return 0;
    }

    if (ip_route_find_by_iface(ip->ip_dst, NULL)) {
        LOG(LOG_WARN, "Invalid destination address");

        if (XSTACK_IP_SEND_HOSTUNREAC) {
            /* TODO Send hostunreac */
        }
        return 0;
    }

    /* Insert to ARP table so it's possible/faster to send a reply. */
    arp_cache_insert(ip->ip_src, hdr->h_src, ARP_CACHE_DYN);

    SET_FOREACH(tmpp, _ip_proto_handlers) {
        proto = *tmpp;
        if (proto->proto_id == ip->ip_proto) {
            break;
        }
        proto = NULL;
    }

    LOG(LOG_DEBUG, "proto id: 0x%x", ip->ip_proto);

    if (proto) {
        int retval;

        /* RFE Support fragments and use hlen + ip->ip_len? */
        retval = proto->fn(ip, payload + hlen, bsize - hlen);
        if (retval > 0) {
            in_addr_t tmp;

            /* Swap source and destination. */
            tmp = ip->ip_src;
            ip->ip_src = ip->ip_dst;
            ip->ip_dst = tmp;

            retval += ip_hdr_hlen(ip);
            ip->ip_len = retval;

            /* Back to network order */
            ip_hton(ip, ip);
            ip->ip_csum = 0;
            ip->ip_csum = ip_checksum(ip, sizeof(struct ip_hdr));
        }
        return retval;
    } else {
        LOG(LOG_INFO, "Unsupported protocol");

        errno = EPROTONOSUPPORT;
        return -1;
    }
    return 0;
}
ETHER_PROTO_INPUT_HANDLER(ETHER_PROTO_IPV4, ip_input);

static struct ip_hdr ip_hdr_template = {
    .ip_vhl = IP_VHL_DEFAULT,
    .ip_tos = IP_TOS_DEFAULT,
    .ip_foff = IP_TOFF_DEFAULT,
    .ip_ttl = IP_TTL_DEFAULT,
};

int ip_send(in_addr_t src, in_addr_t dst, uint8_t proto,
            const uint8_t * buf, size_t bsize)
{
    mac_addr_t dst_mac;
    size_t packet_size = sizeof(struct ip_hdr) + bsize;
    uint8_t packet[packet_size];
    struct ip_hdr * hdr = (struct ip_hdr *)packet;
    struct ip_route route;

    if (ip_route_find_by_iface(src, &route)) {
        LOG(LOG_ERR, "Invalid source address");
        errno = ENXIO;
        return -1;
    }

    if (arp_cache_get_haddr(route.r_iface, dst, dst_mac)) {
        if (errno == EHOSTUNREACH) {
            int retval;
            /*
             * We must defer the operation for now because we are waiting for
             * the reveiver's MAC addr to be resolved.
             */
            retval = ip_defer_push(src, dst, proto, buf, bsize);
            if (retval == 0 || (retval == -1 && errno == EALREADY)) {
                return 0; /* Return 0 to indicate an defered operation. */
            } /* else an error occured. */
        }
        return -1;
    }

    memcpy(hdr, &ip_hdr_template, sizeof(ip_hdr_template));
    hdr->ip_len = packet_size;
    hdr->ip_src = src;
    hdr->ip_dst = dst;
    hdr->ip_proto = proto;
    memcpy(packet + sizeof(ip_hdr_template), buf, bsize);
    ip_hton(hdr, hdr);
    hdr->ip_csum = ip_checksum(hdr, sizeof(struct ip_hdr));

    return ether_send(route.r_iface_handle, dst_mac, ETHER_PROTO_IPV4, packet,
                      packet_size);
}
