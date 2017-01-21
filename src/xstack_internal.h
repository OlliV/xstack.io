#ifndef XSTACK_INTERNAL_H
#define XSTACK_INTERNAL_H

#include <stdatomic.h>
#include <sys/time.h>

#include "tree.h"
#include "xstack_socket.h"

#define XSTACK_CTRL_FLAG_DYING 0x8000

typedef void xstack_periodic_task_t(int delta_time);

/**
 * Declare a periodic task.
 */
#define XSTACK_PERIODIC_TASK(_task_fn_) \
    DATA_SET(_xstack_periodic_tasks, _task_fn_)

struct queue_cb;

/**
 * A generic socket descriptor.
 */
struct xstack_sock {
    /* Static */
    struct xstack_sock_info info;
    char shmem_path[80];
    uint8_t * ingress_data;
    struct queue_cb * ingress_q;
    uint8_t * egress_data;
    struct queue_cb * egress_q;

    /* Runtime */
    union {
        struct {
            RB_ENTRY(xstack_sock) _entry;
        } udp;
    } data;
};

/**
 * Socket datagram wrapping.
 * @{
 */

/**
 * Handle socket input data.
 * Transport -> Socket
 */
int xstack_sock_dgram_input(struct xstack_sock * sock,
                            struct xstack_sockaddr * srcaddr,
                            uint8_t * buf, size_t bsize);

typedef int xstack_send_fn(struct xstack_sock * sock,
                           const struct xstack_dgram * dgram);

/**
 * @}
 */

#endif /* XSTACK_INTERNAL_H */
