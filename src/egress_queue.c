/*
 * Xstack egress packet queue.
 */

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "xstack_internal.h"

static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static fd_set xstack_egress_fds;

__attribute__((constructor)) static void egress_queue_init(void)
{
    FD_ZERO(&xstack_egress_fds);
}

void xstack_egress_add_fd(int fildes)
{
    pthread_mutex_lock(&queue_mutex);
    FD_SET(fildes, &xstack_egress_fds);
    pthread_mutex_unlock(&queue_mutex);
}

void xstack_egress_rm_fd(int fildes)
{
    pthread_mutex_lock(&queue_mutex);
    FD_CLR(fildes, &xstack_egress_fds);
    pthread_mutex_unlock(&queue_mutex);
}

static fd_set fds;
int xstack_wait4egress_packet(struct xstack_send_args * args_out,
                              struct timeval *timeout)
{
    int fd, res;

    /*
     * Wait for outgoing datagrams to be available.
     */
    do {
        /* TODO Nonportable? */
        pthread_mutex_lock(&queue_mutex);
        memcpy(&fds, &xstack_egress_fds, sizeof(fd_set));
        pthread_mutex_unlock(&queue_mutex);

        res = select(FD_SETSIZE, &fds, NULL, NULL, timeout);
    } while (res == -1 && errno == EINTR);
    if (res == -1) {
        return -1;
    }

    /*
     * Iterate through all possible fds because that's the only portable way
     * to figure out which fds are set :(
     */
    for (fd = 0; fd < FD_SETSIZE; ++fd) {
        if (FD_ISSET(fd, &fds)) {
            read(fd, args_out, sizeof(struct xstack_send_args));
            /* TODO If read() returns 0 => EOF */

            return fd;
        }
    }

    return -1;
}
