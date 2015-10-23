#ifndef XSTACK_IP_H
#define XSTACK_IP_H

#include "xstack_in.h"
#include "linker_set.h"

#define IP_STR_LEN  17

/**
 * Local Default Interface
 * @{
 */
int ip_local_ether_handle;
in_addr_t ip_local_addr;
/**
 * @}
 */

/**
 * IP Packet Header.
 */
struct ip_hdr {
    uint8_t ip_vhl;
    uint8_t ip_tos;
    uint16_t ip_len;
    uint16_t ip_id;
    uint16_t ip_foff;
    uint8_t ip_ttl;
    uint8_t ip_proto;
    uint16_t ip_csum;
    uint32_t ip_src;
    uint32_t ip_dst;
    uint8_t ip_opt[0];

} __attribute__((packed, aligned(4)));

/**
 * IP Packet Header Defaults
 */
/* v4 and 5 * 4 octets */
#define IP_VHL_DEFAULT  0x45    /*!< Default value for version and ihl. */
#define IP_TOS_DEFAULT  0x0     /*!< Default type of service and no ECN */
#define IP_TOFF_DEFAULT 0x4000
#define IP_TTL_DEFAULT 64
/**
 * @}
 */

/**
 * IP protocol numbers.
 * @{
 */
#define IP_PROTO_ICMP     1
#define IP_PROTO_IGMP     2
#define IP_PROTO_TCP      6
#define IP_PROTO_UDP     17
#define IP_PROTO_SCTP   132
/**
 * @}
 */

struct _ip_proto_handler {
    uint16_t proto_id;
    void (*fn)(const struct ip_hdr * hdr, uint8_t * payload, size_t bsize);
};

/**
 * Declare an IP input chain handler.
 */
#define IP_PROTO_INPUT_HANDLER(_proto_id_, _handler_fn_)                    \
    static struct _ip_proto_handler _ip_proto_handler_##_handler_fn_ = {    \
        .proto_id = _proto_id_,                                             \
        .fn = _handler_fn_,                                                 \
    };                                                                      \
    DATA_SET(_ip_proto_handlers, _ip_proto_handler_##_handler_fn_)

typedef void ip_periodic_task_t(int delta_time);

/**
 * Declare a periodic IP task.
 */
#define IP_PERIODIC_TASK(_task_fn_) \
    DATA_SET(_ip_periodic_tasks, _task_fn_)

int ip_config(int ether_handle, in_addr_t ip_addr);

/**
 * Convert an IP address from integer representation to a C string.
 * @note The minimum size of buf is IP_STR_LEN.
 * @param[in] ip is the IP address to be converted.
 * @param[out] buf is the destination buffer.
 */
void ip2str(in_addr_t ip, char * buf);

/**
 * Calculate the Internet checksum.
 */
uint16_t ip_checksum(void * dp, size_t bsize);

/**
 * Get the header length of an IP packet.
 */
static inline size_t ip_hdr_hlen(const struct ip_hdr * ip)
{
    return (ip->ip_vhl & 0x0f) * 4;
}

/**
 * Send an IP packet to a destination.
 */
int ip_send(in_addr_t src, in_addr_t dst, uint8_t proto,
            const uint8_t * buf, size_t bsize);

void ip_run_periodic_tasks(int delta_time);

#endif /* XSTACK_IP_H */
