#include <errno.h>
#include <string.h>

#include "xstack_ether.h"
#include "xstack_in.h"
#include "xstack_ip.h"

#include "logger.h"

struct ip_defer {
    int tries;
    in_addr_t dst;
    uint8_t proto;
    size_t buf_size;
    uint8_t buf[ETHER_ALEN];
};

/*
 * This is used to inhibit defers if it's the ip deferring code itself causing
 * defer push.
 */
static unsigned defer_inhibit;

static struct ip_defer ip_defer_queue[XSTACK_IP_DEFER_MAX];
static size_t q_rd, q_wr;

int ip_defer_push(in_addr_t dst, uint8_t proto,
                  const uint8_t * buf, size_t bsize)
{
    const size_t next = (q_wr + 1) % num_elem(ip_defer_queue);
    struct ip_defer * slot;

    if (defer_inhibit) {
        errno = EALREADY;
        return -1;
    }

    if (next == q_rd) {
        errno = ENOBUFS;
        return -1;
    }
    slot = ip_defer_queue + q_wr;

    if (bsize > ETHER_ALEN) {
        errno = EMSGSIZE;
        return -1;
    }

    slot->tries = 0;
    slot->dst = dst;
    slot->proto = proto;
    memcpy(slot->buf, buf, bsize);

    q_wr = next;
    return 0;
}

static struct ip_defer * ip_defer_peek(void)
{
    if (q_rd == q_wr) {
        return NULL;
    }

    return ip_defer_queue + q_rd;
}

static void ip_defer_drop(void)
{
    if (q_rd == q_wr) {
        return;
    }

    q_rd = (q_rd + 1) % num_elem(ip_defer_queue);
}

void ip_defer_handler(int delta_time __unused)
{
    struct ip_defer * ipd;

    defer_inhibit = 1;
    while (1) {
        ipd = ip_defer_peek();
        if (!ipd) {
            return;
        }

        if (ipd->tries++ > 3) { /* After couple of tries we must drop it. */
            char str_ip[IP_STR_LEN];

            ip2str(ipd->dst, str_ip);
            LOG(LOG_INFO, "Dropping IP deferred transmission for %s", str_ip);
            ip_defer_drop();
            continue;
        }

        if (ip_send(ipd->dst, ipd->proto, ipd->buf, ipd->buf_size) == -1) {
            if (errno == EHOSTUNREACH) {
                ipd->tries++; /* Try again later. */
                return;
            }
        }
        ip_defer_drop();
    }
    defer_inhibit = 0;
}
IP_PERIODIC_TASK(ip_defer_handler);
