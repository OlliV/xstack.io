#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>

#include "xstack_socket.h"
#include "xstack_util.h"

/* TODO bind fn for outbound connections */

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
    return pa;
}

/* TODO connection cache */
static struct xstack_sock_info last_conn;

ssize_t xstack_recvfrom(void * socket, void * restrict buffer, size_t length,
                        int flags, struct xstack_sockaddr * restrict address)
{
    struct queue_cb * ingress_q = XSTACK_INGRESS_QADDR(socket);
    struct xstack_dgram * dgram;
    int dgram_index;
    ssize_t rd;

    /* Implement signaling */
    while(!queue_peek(ingress_q, &dgram_index));
    dgram = (struct xstack_dgram *)(XSTACK_INGRESS_DADDR(socket) + dgram_index);

    /* TODO Implement flags */

    last_conn = dgram->sock_info;
    if (address)
        *address = dgram->addr;
    rd = smin(length, dgram->buf_size);
    memcpy(buffer, dgram->buf, rd);
    dgram = NULL;

    queue_discard(ingress_q, 1);

    return rd;
}

ssize_t xstack_sendto(void * socket, const void * buffer, size_t length,
                      int flags, const struct xstack_sockaddr * dest_addr)
{
    struct queue_cb * egress_q = XSTACK_EGRESS_QADDR(socket);
    struct xstack_dgram * dgram;
    int dgram_index;

    if (length > XSTACK_DATAGRAM_SIZE_MAX) {
        errno = ENOBUFS;
        return -1;
    }

    while ((dgram_index = queue_alloc(egress_q)) == -1);
    dgram = (struct xstack_dgram *)(XSTACK_EGRESS_DADDR(socket) + dgram_index);

    dgram->sock_info = last_conn;
    dgram->addr = *dest_addr;
    memcpy(dgram->buf, buffer, length);

    queue_commit(egress_q);
    kill(last_conn.pid_inetd, SIGSTOP);

    return length;
}
