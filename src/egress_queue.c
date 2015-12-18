/*
 * Xstack egress packet queue.
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "logger.h"
#include "xstack_internal.h"

static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static int egress_fds[FD_SETSIZE];
static fd_set egress_fd_set;

__attribute__((constructor)) static void egress_queue_init(void)
{
    size_t i;

    for (i = 0; i < num_elem(egress_fds); i++) {
        egress_fds[i] = -1;
    }
    FD_ZERO(&egress_fd_set);
}

static int fd_comp(const void * a, const void * b)
{
    return *(int *)b - *(int *)a;
}

static void sort_egress_fds(void)
{
    qsort(egress_fds, num_elem(egress_fds), sizeof(int), fd_comp);
}

static void update_fd_set(void)
{
    size_t i;

    FD_ZERO(&egress_fd_set);

    for (i = 0; i < num_elem(egress_fds); i++) {
        int fildes = egress_fds[i];

        if (fildes == -1) {
            break;
        }
        FD_SET(fildes, &egress_fd_set);
    }
}

void xstack_egress_add_fd(int fildes)
{
    int i = num_elem(egress_fds) - 1;

    pthread_mutex_lock(&queue_mutex);
    if (egress_fds[i] == -1) {
        egress_fds[i] = fildes;
        sort_egress_fds();
    } else {
        LOG(LOG_ERR, "Out of fds");
    }
    update_fd_set();
    pthread_mutex_unlock(&queue_mutex);
}

void xstack_egress_rm_fd(int fildes)
{
    int * fdp;

    pthread_mutex_lock(&queue_mutex);
    fdp = bsearch(&fildes, egress_fds, num_elem(egress_fds), sizeof(int),
                  fd_comp);
    *fdp = -1;
    sort_egress_fds();
    update_fd_set();
    pthread_mutex_unlock(&queue_mutex);
}

static int wait_fds[FD_SETSIZE];
static fd_set wait_fd_set;
int xstack_wait4egress_packet(struct xstack_send_args * args_out,
                              struct timeval *timeout)
{
    int res;

    /*
     * Wait for outgoing datagrams to be available.
     */
    do {
        /* TODO Nonportable? */
        pthread_mutex_lock(&queue_mutex);
        memcpy(&wait_fds, &egress_fds, sizeof(egress_fds));
        memcpy(&wait_fd_set, &egress_fd_set, sizeof(fd_set));
        pthread_mutex_unlock(&queue_mutex);

        res = select(FD_SETSIZE, &wait_fd_set, NULL, NULL, timeout);
    } while (res == -1 && errno == EINTR);
    if (res == -1) {
        return -1;
    }

    for (size_t i = 0; i < num_elem(wait_fds) && wait_fds[i] != -1; i++) {
        int fd = wait_fds[i];

        if (FD_ISSET(fd, &wait_fd_set)) {
            read(fd, args_out, sizeof(struct xstack_send_args));
        }
    }

    return -1;
}
