#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "linker_set.h"
#include "xstack.h"
#include "xstack_ether.h"
#include "xstack_ip.h"
#include "xstack_socket.h"

#include "logger.h"
#include "udp.h"
#include "xstack_internal.h"

enum xstack_state {
    XSTACK_STOPPED = 0,
    XSTACK_RUNNING,
    XSTACK_DYING,
};

SET_DECLARE(_xstack_periodic_tasks, void);

static pthread_mutex_t xstack_mutex = PTHREAD_MUTEX_INITIALIZER;
static enum xstack_state xstack_state = XSTACK_STOPPED;
static int ether_handle;
static pthread_t ingress_tid, egress_tid;
static fd_set xstack_egress_fds;

__attribute__((constructor)) static void xstack_init(void)
{
    FD_ZERO(&xstack_egress_fds);
}

static enum xstack_state get_state(void)
{
    enum xstack_state state;

    state = xstack_state;

   return state;
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

static void run_periodic_tasks(int delta_time)
{
    void ** taskp;

    SET_FOREACH(taskp, _xstack_periodic_tasks) {
        xstack_periodic_task_t * task = *(xstack_periodic_task_t **)taskp;

        if (task)
            task(delta_time);
    }
}

static void block_sigpipe(void)
{
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGPIPE);

    if (pthread_sigmask(SIG_BLOCK, &sigset, NULL) == -1) {
        LOG(LOG_ERR, "Unable to ignore SIGPIPE");
        abort();
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

    block_sigpipe();

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
                if (errno == EPIPE) {
                    /* EPIPE means that the other end is closed. */
                    /* TODO Handle closing of the pipe */
                } else {
                    LOG(LOG_ERR, "Protocol handling failed: %d", errno);
                }
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

void xstack_egress_add_fd(int fildes)
{
    pthread_mutex_lock(&xstack_mutex);
    FD_SET(fildes, &xstack_egress_fds);
    pthread_mutex_unlock(&xstack_mutex);
}

void xstack_egress_rm_fd(int fildes)
{
    pthread_mutex_lock(&xstack_mutex);
    FD_CLR(fildes, &xstack_egress_fds);
    pthread_mutex_unlock(&xstack_mutex);
}

/**
 * Handle the egress traffic.
 * All egress traffic is mux'd and serialized through one egress pipe.
 * socket fd -> transport
 */
static void * xstack_egress_thread(void * arg)
{
    static fd_set fds;
    int res;

    while (1) {
        struct timeval timeout = {
            .tv_sec = XSTACK_PERIODIC_EVENT_SEC,
            .tv_usec = 0,
        };

        /*
         * Wait for outgoing datagrams to be available.
         */
        do {
            pthread_mutex_lock(&xstack_mutex);
            /* TODO Nonportable? */
            memcpy(&fds, &xstack_egress_fds, sizeof(fd_set));
            pthread_mutex_unlock(&xstack_mutex);

            res = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
        } while (res == -1 && errno == EINTR);

        /*
         * Iterate through all possible fds because that's the only portable way
         * to figure out which fds are set :(
         */
        for (size_t fd = 0; fd < FD_SETSIZE; ++fd) {
            if (FD_ISSET(fd, &fds)) {
                struct xstack_send_args args;

                read(fd, &args, sizeof(args));
                /* TODO If read() returns 0 => EOF */

                LOG(LOG_DEBUG, "Sending a datagram");
                switch (args.sock->sock_proto) {
                case XIP_PROTO_UDP:
                    if (args.buf_size > 0 &&  args.buf_size < UDP_MAXLEN &&
                        xstack_udp_send(fd, &args) >= 0) {
                        break;
                    }
                    /* Fallthrough */
                default:
                    LOG(LOG_ERR, "Failed to send a datagram");
                }
            }
        }

        if (get_state() == XSTACK_DYING) {
            break;
        }
    }

    pthread_exit(NULL);
}

int xstack_start(int handle)
{
    ether_handle = handle;

    pthread_mutex_lock(&xstack_mutex);
    if (get_state() != XSTACK_STOPPED) {
        errno = EALREADY;
        goto fail;
    }

    if (pthread_create(&ingress_tid, NULL, xstack_ingress_thread, NULL)) {
        goto fail;
    }

    if (pthread_create(&egress_tid, NULL, xstack_egress_thread, NULL)) {
        pthread_cancel(ingress_tid);
        goto fail;
    }

    pthread_setname_np(ingress_tid, "xstack ingress");
    pthread_setname_np(egress_tid, "xstack ingress");

    set_state(XSTACK_RUNNING);
    pthread_mutex_unlock(&xstack_mutex);
    return 0;
fail:
    pthread_mutex_unlock(&xstack_mutex);
    return -1;
}

void xstack_stop(void)
{
    set_state(XSTACK_DYING);

    pthread_join(ingress_tid, NULL);
    pthread_join(egress_tid, NULL);

    set_state(XSTACK_STOPPED);
}
