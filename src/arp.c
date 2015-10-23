#include <errno.h>
#include <string.h>
#include "xstack_arp.h"
#include "xstack_ether.h"
#include "xstack_ip.h"
#include "util.h"
#include "logger.h"

#define ARP_CACHE_AGE_MAX (20 * 60 * 60) /* Expiration time */

struct arp_cache_entry {
    in_addr_t ip_addr;
    mac_addr_t haddr;
    int age; /* 0 = free slot; -1 = static */
};

static struct arp_cache_entry arp_cache[10]; /* TODO Configurable size */

static int arp_request(int ether_handle, in_addr_t spa, in_addr_t tpa);

static void arp_hton(const struct arp_ip * arp_host, struct arp_ip * arp_net)
{
    arp_net->arp_htype = htons(arp_host->arp_htype);
    arp_net->arp_ptype = htons(arp_host->arp_ptype);
    arp_net->arp_hlen = arp_host->arp_hlen;
    arp_net->arp_plen = arp_host->arp_plen;
    arp_net->arp_oper = htons(arp_host->arp_oper);
    memmove(arp_net->arp_sha, arp_host->arp_sha, sizeof(mac_addr_t));
    arp_net->arp_spa = ntohl(arp_host->arp_spa);
    memmove(arp_net->arp_tha, arp_host->arp_tha, sizeof(mac_addr_t));
    arp_net->arp_tpa = ntohl(arp_host->arp_tpa);
}

static void arp_ntoh(const struct arp_ip * arp_net, struct arp_ip * arp_host)
{
    arp_host->arp_htype = htons(arp_net->arp_htype);
    arp_host->arp_ptype = htons(arp_net->arp_ptype);
    arp_host->arp_hlen = arp_net->arp_hlen;
    arp_host->arp_plen = arp_net->arp_plen;
    arp_host->arp_oper = htons(arp_net->arp_oper);
    memmove(arp_host->arp_sha, arp_net->arp_sha, sizeof(mac_addr_t));
    arp_host->arp_spa = ntohl(arp_net->arp_spa);
    memmove(arp_host->arp_tha, arp_net->arp_tha, sizeof(mac_addr_t));
    arp_host->arp_tpa = ntohl(arp_net->arp_tpa);
}

int arp_cache_insert(in_addr_t ip_addr, mac_addr_t haddr,
                     enum arp_cache_entry_type type)
{
    size_t i;
    struct arp_cache_entry * it;
    struct arp_cache_entry * entry = NULL;

    it = arp_cache;
    for (i = 0; i < num_elem(arp_cache); i++) {
        if (it->age == 0) {
            entry = it;
        } else if ((entry && entry->age > it->age) ||
                   (!entry && it->age > 0)) {
            entry = it;
        }
        if (it->ip_addr == ip_addr) {
            /* This is a replacement for an existing entry. */
            entry = it;
            break;
        }
        it++;
    }
    if (!entry) {
        errno = ENOMEM;
        return -1;
    }

    entry->ip_addr = ip_addr;
    memcpy(entry->haddr, haddr, sizeof(mac_addr_t));
    entry->age = (int)type;

    return 0;
}

static struct arp_cache_entry * arp_cache_get_entry(in_addr_t ip_addr)
{
    size_t i;
    struct arp_cache_entry * entry = NULL;

    for (i = 0; i < num_elem(arp_cache); i++) {
        entry = &arp_cache[i];

        if (entry->ip_addr == ip_addr) {
            break;
        }
    }

    return entry;
}

void arp_cache_remove(in_addr_t ip_addr)
{
    struct arp_cache_entry * entry = arp_cache_get_entry(ip_addr);

    if (entry)
        entry->age = 0;
}

int arp_cache_get_haddr(in_addr_t ip_addr, mac_addr_t haddr)
{
    struct arp_cache_entry * entry = arp_cache_get_entry(ip_addr);

    if (entry) {
        memcpy(haddr, entry->haddr, sizeof(mac_addr_t));
        return 0;
    }

    /* TODO Loop through each interface? */
    /* TODO How to wait for the reply? */
    if (!arp_request(ip_local_ether_handle, ip_local_addr, ip_addr)) {
        //arp_cache_insert(ip_addr, haddr, ARP_CACHE_DYN);
        //return 0;
    }

    errno = EHOSTUNREACH;
    return -1;
}

static void arp_cache_update(int delta_time)
{
    size_t i;

    for (i = 0; i < num_elem(arp_cache); i++) {
        struct arp_cache_entry * entry = &arp_cache[i];

        if (entry->age > ARP_CACHE_AGE_MAX) {
            entry->age = 0;
        } else if (entry->age > 0) {
            entry->age += delta_time;
        }
    }
}
IP_PERIODIC_TASK(arp_cache_update);

static void arp_input(const struct ether_hdr * hdr, uint8_t * payload, size_t bsize)
{
    struct arp_ip * arp_net = (struct arp_ip *)payload;
    struct arp_ip arp;

    arp_ntoh(arp_net, &arp);

    if (arp.arp_htype != ARP_HTYPE_ETHER) {
        return;
    }

    if (arp.arp_ptype == ETHER_PROTO_IPV4) {
        char str_ip[IP_STR_LEN];

        /* Add sender to the ARP cache */
        arp_cache_insert(arp.arp_spa, arp.arp_sha, ARP_CACHE_DYN);

        /* Process the opcode */
        switch (arp.arp_oper) {
        case ARP_OPER_REQUEST:
            ip2str(arp.arp_tpa, str_ip);
            LOG(LOG_DEBUG, "ARP request: %s", str_ip);

            if (arp.arp_tpa == ip_local_addr) {
                arp_net->arp_oper = htons(ARP_OPER_REPLY);
                ether_handle2addr(ip_local_ether_handle, arp_net->arp_sha);
                memcpy(arp_net->arp_tha, arp.arp_sha, sizeof(mac_addr_t));
                arp_net->arp_tpa = arp_net->arp_spa;
                arp_net->arp_spa = htonl(ip_local_addr);

                if (ether_send(ip_local_ether_handle, hdr->h_src,
                               ETHER_PROTO_ARP, payload, bsize) < 0) {
                    LOG(LOG_WARN, "ARP reply failed");
                }
            }
            break;
        case ARP_OPER_REPLY:
            /* Nothing more to do. */
        default:
            LOG(LOG_WARN, "Invalid ARP op: %d", arp.arp_oper);
            return;
        }
    } else {
        LOG(LOG_DEBUG, "Unknown ptype");
    }
}
ETHER_PROTO_INPUT_HANDLER(ETHER_PROTO_ARP, arp_input);

/**
 * @param[in] sha
 * @param[in] spa
 * @param[in] tpa
 * @param[out] tha
 */
static int arp_request(int ether_handle, in_addr_t spa, in_addr_t tpa)
{
    struct arp_ip msg = {
        .arp_htype = ARP_HTYPE_ETHER,
        .arp_ptype = ETHER_PROTO_IPV4,
        .arp_hlen = ETHER_ALEN,
        .arp_plen = sizeof(in_addr_t),
        .arp_oper = ARP_OPER_REQUEST,
        .arp_spa = spa,
        .arp_tpa = tpa,
    };

    ether_handle2addr(ether_handle, msg.arp_sha);
    memset(msg.arp_tha, 0, sizeof(mac_addr_t));

    arp_hton(&msg, &msg);
    return ether_send(ether_handle, mac_broadcast_addr, ETHER_PROTO_ARP,
                      (uint8_t *)(&msg), sizeof(msg));
}

int arp_gratuitous(int ether_handle, in_addr_t spa)
{
    struct arp_ip msg = {
        .arp_htype = ARP_HTYPE_ETHER,
        .arp_ptype = ETHER_PROTO_IPV4,
        .arp_hlen = ETHER_ALEN,
        .arp_plen = sizeof(in_addr_t),
        .arp_oper = ARP_OPER_REQUEST,
        .arp_spa = spa,
        .arp_tpa = spa,
    };
    int retval;
    char str_ip[IP_STR_LEN];

    ether_handle2addr(ether_handle, msg.arp_sha);
    memset(msg.arp_tha, 0, sizeof(mac_addr_t));

    ip2str(spa, str_ip);
    LOG(LOG_DEBUG, "Announce %s", str_ip);

    arp_hton(&msg, &msg);
    retval = ether_send(ether_handle, mac_broadcast_addr, ETHER_PROTO_ARP,
                        (uint8_t *)(&msg), sizeof(msg));
    if (retval == -1) {
        char errmsg[40];

        strerror_r(errno, errmsg, sizeof(errmsg));
        LOG(LOG_WARN, "Failed to announce %s: %s (%d)", str_ip, errmsg, retval);
    }

    return retval;
}
