#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "linker_set.h"
#include "xstack.h"
#include "xstack_ether.h"
#include "xstack_ip.h"
#include "xstack_socket.h"

#include "logger.h"
#include "udp.h"
#include "xstack_internal.h"

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
static int ether_handle;
static pthread_t ingress_tid, egress_tid;

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

xstack_send_fn * proto_send[] = {
    [XIP_PROTO_UDP] = xstack_udp_send,
};

/**
 * Handle the egress traffic.
 * All egress traffic is mux'd and serialized through one egress pipe.
 * socket fd -> transport
 */
static void * xstack_egress_thread(void * arg)
{
    block_sigpipe();

    while (1) {
        int fd;
        struct xstack_cmsg_dgram_send args;
        struct timeval timeout = {
            .tv_sec = XSTACK_PERIODIC_EVENT_SEC,
            .tv_usec = 0,
        };

        fd = xstack_wait4egress_packet(&args, &timeout);
        if (fd != -1) {
            enum xstack_sock_proto proto = args.sock->sock_proto;

            LOG(LOG_DEBUG, "Sending a datagram");
            if (args.sock->sock_proto > XIP_PROTO_NONE &&
                args.sock->sock_proto < XIP_PROTO_LAST) {
                if (proto_send[proto](fd, &args) < 0) {
                    LOG(LOG_ERR, "Failed to send a datagram");
                }
            } else {
                LOG(LOG_ERR, "Invalid protocol");
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

    if (get_state() != XSTACK_STOPPED) {
        errno = EALREADY;
        return -1;
    }

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
