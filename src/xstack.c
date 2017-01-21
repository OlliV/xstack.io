#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "linker_set.h"
#include "xstack_ether.h"
#include "xstack_in.h"
#include "xstack_socket.h"

#include "logger.h"
#include "queue.h"
#include "udp.h"
#include "xstack_internal.h"
#include "xstack_ip.h"

/**
 * Xstack ingress and egress thread state.
 */
enum xstack_state {
    XSTACK_STOPPED = 0, /*!< Ingress and egress threads are not running. */
    XSTACK_RUNNING,     /*!< Ingress and egress threads are running. */
    XSTACK_DYING,       /*!< Waiting for ingress and engress threads to stop. */
};

SET_DECLARE(_xstack_periodic_tasks, void);

/*
 * Xstack state variables.
 */
static enum xstack_state xstack_state = XSTACK_STOPPED;
static pthread_t ingress_tid, egress_tid;
static int ether_handle;

static xstack_send_fn * proto_send[] = {
    [XIP_PROTO_UDP] = xstack_udp_send,
};

static struct xstack_sock sockets[] = {
    {
        .info.sock_dom = XF_INET4,
        .info.sock_type = XSOCK_DGRAM,
        .info.sock_proto = XIP_PROTO_UDP,
        .info.sock_addr = (struct xstack_sockaddr){
            .inet4_addr = 167772162,
            .port = 10,
        },
        .shmem_path = "/tmp/utelnet.sock",
    },
};

static enum xstack_state get_state(void)
{
    enum xstack_state * state = &xstack_state;

    return *state;
}

static void set_state(enum xstack_state state)
{
    xstack_state = state;
}

static int delta_time;
static int eval_timer(void)
{
    static struct timespec start;
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);
    delta_time = now.tv_sec - start.tv_sec;
    if (delta_time >= XSTACK_PERIODIC_EVENT_SEC) {
        start = now;
        return !0;
    }
    return 0;
}

/**
 * Bind an address to a socket.
 * @param[in] sock is a pointer to the socket returned by xstack_socket().
 * @returns Uppon succesful completion returns 0;
 *          Otherwise -1 is returned and errno is set.
 */
static int xstack_bind(struct xstack_sock * sock)
{
    switch (sock->info.sock_proto) {
    case XIP_PROTO_UDP:
        return xstack_udp_bind(sock);
    default:
        errno = EPROTOTYPE;
        return -1;
    }
}

static void run_periodic_tasks(int delta_time)
{
    void ** taskp;

    SET_FOREACH(taskp, _xstack_periodic_tasks) {
        xstack_periodic_task_t * task = *(xstack_periodic_task_t **)taskp;

        if (task)
            task(delta_time);
    }
}

/**
 * Handle the ingress traffic.
 * All ingress data is handled in a single pipeline until this point where
 * the data is demultiplexed to sockets.
 * transport -> socket fd
 */
static void * xstack_ingress_thread(void * arg)
{
    static uint8_t rx_buffer[ETHER_MAXLEN];

    while (1) {
        struct ether_hdr hdr;
        int retval;

        LOG(LOG_DEBUG, "Waiting for rx");

        retval = ether_receive(ether_handle, &hdr, rx_buffer,
                               sizeof(rx_buffer));
        if (retval == -1) {
            LOG(LOG_ERR, "Rx failed: %d", errno);
        } else if (retval > 0) {
            LOG(LOG_DEBUG, "Frame received!");

            retval = ether_input(&hdr, rx_buffer, retval);
            if (retval == -1) {
                LOG(LOG_ERR, "Protocol handling failed: %d", errno);
            } else if (retval > 0) {
                retval = ether_output_reply(ether_handle, &hdr, rx_buffer,
                                            retval);
                if (retval < 0) {
                    LOG(LOG_ERR, "Reply failed: %d", errno);
                }
            }
        }

        if (eval_timer()) {
            LOG(LOG_DEBUG, "tick");
            run_periodic_tasks(delta_time);
        }

        if (get_state() == XSTACK_DYING) {
            break;
        }
    }

    pthread_exit(NULL);
}

/**
 * Handle the egress traffic.
 * All egress traffic is mux'd and serialized through one egress pipe.
 * socket fd -> transport
 */
static void * xstack_egress_thread(void * arg)
{
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR2);

    while (1) {
        struct timespec timeout = {
            .tv_sec = XSTACK_PERIODIC_EVENT_SEC,
            .tv_nsec = 0,
        };

        sigtimedwait(&sigset, NULL, &timeout);

        for (size_t i = 0; i < num_elem(sockets); i++) {
            struct xstack_sock * sock = sockets + i;

            if (!queue_isempty(sock->egress_q)) {
                int dgram_index;
                struct xstack_dgram * dgram;
                enum xstack_sock_proto proto;

                while(!queue_peek(sock->egress_q, &dgram_index));
                dgram = (struct xstack_dgram *)(sock->egress_data +
                                                dgram_index);

                /* TODO check that the protocol matches with the socket */
                LOG(LOG_DEBUG, "Sending a datagram");
                proto = sock->info.sock_proto;
                if (proto > XIP_PROTO_NONE &&
                    proto < XIP_PROTO_LAST) {
                    if (proto_send[proto](sock, dgram) < 0) {
                        LOG(LOG_ERR, "Failed to send a datagram");
                    }
                } else {
                    LOG(LOG_ERR, "Invalid protocol");
                }

                queue_discard(sock->egress_q, 1);
            }
        }

        if (get_state() == XSTACK_DYING) {
            break;
        }
    }

    pthread_exit(NULL);
}

static void xstack_init(void)
{
    pid_t mypid = getpid();

    for (size_t i = 0; i < num_elem(sockets); i++) {
        struct xstack_sock * sock = sockets + i;
        int fd;
        void * pa;

        sock->info.pid_inetd = mypid;

        fd = open(sock->shmem_path, O_RDWR);
        if (fd == -1) {
            perror("Failed to open shmem file");
            exit(1);
        }

        pa = mmap(0, XSTACK_SHMEM_SIZE, PROT_READ | PROT_WRITE,
                  MAP_SHARED, fd, 0);
        if (pa == MAP_FAILED) {
            perror("Failed to mmap() shared mem");
            exit(1);
        }
        memset(pa, 0, XSTACK_SHMEM_SIZE);

        sock->ingress_data = XSTACK_INGRESS_DADDR(pa);
        sock->ingress_q = XSTACK_INGRESS_QADDR(pa);
        *sock->ingress_q = queue_create(XSTACK_DATAGRAM_SIZE_MAX,
                                        XSTACK_DATAGRAM_BUF_SIZE);

        sock->egress_data = XSTACK_EGRESS_DADDR(pa);
        sock->egress_q = XSTACK_EGRESS_QADDR(pa);
        *sock->egress_q = queue_create(XSTACK_DATAGRAM_SIZE_MAX,
                                       XSTACK_DATAGRAM_BUF_SIZE);

        if (xstack_bind(sock) < 0) {
            perror("Failed to bind a socket");
            exit(1);
        }
    }
}

int xstack_start(int handle)
{
    ether_handle = handle;

    if (get_state() != XSTACK_STOPPED) {
        errno = EALREADY;
        return -1;
    }

    xstack_init();

    if (pthread_create(&ingress_tid, NULL, xstack_ingress_thread, NULL)) {
        return -1;
    }

    if (pthread_create(&egress_tid, NULL, xstack_egress_thread, NULL)) {
        pthread_cancel(ingress_tid);
        return -1;
    }

    set_state(XSTACK_RUNNING);
    return 0;
}

void xstack_stop(void)
{
    set_state(XSTACK_DYING);

    pthread_join(ingress_tid, NULL);
    pthread_join(egress_tid, NULL);

    set_state(XSTACK_STOPPED);
}

int xstack_sock_dgram_input(struct xstack_sock * sock,
                            struct xstack_sockaddr * srcaddr,
                            uint8_t * buf, size_t bsize)
{
    int dgram_index;
    struct xstack_dgram * dgram;

    while ((dgram_index = queue_alloc(sock->ingress_q)) == -1);
    dgram = (struct xstack_dgram *)(sock->ingress_data + dgram_index);

    dgram->sock_info = sock->info;
    dgram->addr = *srcaddr;
    dgram->buf_size = bsize;
    memcpy(dgram->buf, buf, bsize);

    queue_commit(sock->ingress_q);

    return 0;
}

int main(int argc, char * argv[])
{
    char * const ether_args[] = {
        argv[1],
        NULL,
    };
    int handle;
    sigset_t sigset;

    if (argc == 1) {
        fprintf(stderr, "Usage: %s INTERFACE\n", argv[0]);
        exit(1);
    }

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);

    /* Block sigset for all future threads */
    sigprocmask(SIG_SETMASK, &sigset, NULL);

    handle = ether_init(ether_args);
    if (handle == -1) {
        perror("Failed to init");
        exit(1);
    }

    if (ip_config(handle, 167772162, 4294967040)) {
        perror("Failed to config IP");
        exit(1);
    }

    xstack_start(handle);

    sigwaitinfo(&sigset, NULL);

    fprintf(stderr, "Stopping the IP stack...\n");

    xstack_stop();

    ether_deinit(handle);

    return 0;
}
