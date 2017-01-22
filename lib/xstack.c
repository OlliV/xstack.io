#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "xstack_socket.h"
#include "xstack_util.h"

/* TODO bind fn for outbound connections */

static void block_sigusr2(void)
{
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR2);

    if (pthread_sigmask(SIG_BLOCK, &sigset, NULL) == -1) {
        abort();
    }
}

void * xstack_listen(const char * socket_path)
{
    int fd;
    void * pa;

    fd = open(socket_path, O_RDWR);
    if (fd == -1) {
        return NULL;
    }

    pa = mmap(0, XSTACK_SHMEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (pa == MAP_FAILED) {
        return NULL;
    }

    block_sigusr2();
    XSTACK_SOCK_CTRL(pa)->pid_end = getpid();

    return pa;
}

ssize_t xstack_recvfrom(void * socket, void * restrict buffer, size_t length,
                        int flags, struct xstack_sockaddr * restrict address)
{
    struct queue_cb * ingress_q = XSTACK_INGRESS_QADDR(socket);
    struct xstack_dgram * dgram;
    sigset_t sigset;
    int dgram_index;
    ssize_t rd;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR2);

    do {
        struct timespec timeout = {
            .tv_sec = XSTACK_PERIODIC_EVENT_SEC,
            .tv_nsec = 0,
        };

        sigtimedwait(&sigset, NULL, &timeout);
    } while (!queue_peek(ingress_q, &dgram_index));
    dgram = (struct xstack_dgram *)(XSTACK_INGRESS_DADDR(socket) + dgram_index);

    if (address) {
        *address = dgram->srcaddr;
    }
    rd = smin(length, dgram->buf_size);
    memcpy(buffer, dgram->buf, rd);
    dgram = NULL;

    if (!(flags & XSTACK_MSG_PEEK)) {
        queue_discard(ingress_q, 1);
    }

    return rd;
}

ssize_t xstack_sendto(void * socket, const void * buffer, size_t length,
                      int flags, const struct xstack_sockaddr * dest_addr)
{
    const struct xstack_sock_ctrl * ctrl = XSTACK_SOCK_CTRL(socket);
    struct queue_cb * egress_q = XSTACK_EGRESS_QADDR(socket);
    struct xstack_dgram * dgram;
    int dgram_index;

    if (length > XSTACK_DATAGRAM_SIZE_MAX) {
        errno = ENOBUFS;
        return -1;
    }

    while ((dgram_index = queue_alloc(egress_q)) == -1);
    dgram = (struct xstack_dgram *)(XSTACK_EGRESS_DADDR(socket) + dgram_index);

    /* Ignored by the implementation */
    memset(&dgram->srcaddr, 0, sizeof(struct xstack_sockaddr));

    dgram->dstaddr = *dest_addr;
    memcpy(dgram->buf, buffer, length);

    queue_commit(egress_q);
    kill(ctrl->pid_inetd, SIGUSR2);

    return length;
}
