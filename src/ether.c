#include "xstack_ether.h"
#include "logger.h"

SET_DECLARE(_ether_proto_handlers, struct _ether_proto_handler);

const mac_addr_t mac_broadcast_addr = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

void ether_input(const struct ether_hdr * hdr, uint8_t * payload, size_t bsize)
{
    struct _ether_proto_handler ** tmpp;
    struct _ether_proto_handler * proto;

    SET_FOREACH(tmpp, _ether_proto_handlers) {
        proto = *tmpp;
        if (proto->proto_id == hdr->h_proto) {
            break;
        }
        proto = NULL;
    }

    LOG(LOG_DEBUG, "proto id: 0x%x", hdr->h_proto);

    if (proto) {
        proto->fn(hdr, payload, bsize);
    } else {
        /* TODO Error handling */
        LOG(LOG_WARN, "Unsupported protocol");
    }
}
