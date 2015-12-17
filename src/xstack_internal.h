#ifndef XSTACK_INTERNAL_H
#define XSTACK_INTERNAL_H

#include <sys/time.h>

#include "xstack_socket.h"

struct xstack_send_args {
    struct xstack_sock * sock;
    struct xstack_sockaddr dstaddr; /* For stateless protocols */
    size_t buf_size;
    unsigned flags;
};

typedef void xstack_periodic_task_t(int delta_time);

/**
 * Declare a periodic task.
 */
#define XSTACK_PERIODIC_TASK(_task_fn_) \
    DATA_SET(_xstack_periodic_tasks, _task_fn_)

/**
 * Xstack egress packet queue management.
 * @{
 */
void xstack_egress_add_fd(int fildes);
void xstack_egress_rm_fd(int fildes);
int xstack_wait4egress_packet(struct xstack_send_args * args_out,
                              struct timeval *timeout);
/**
 *
 */

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

/**
 * @}
 */

#endif /* XSTACK_INTERNAL_H */
