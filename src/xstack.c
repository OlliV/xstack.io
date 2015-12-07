#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "xstack.h"
#include "xstack_ether.h"
#include "xstack_ip.h"
#include "xstack_socket.h"

static int ether_handle;
static uint8_t rx_buffer[ETHER_MAXLEN];
static pthread_t xstack_thread_id;
static int stop_xstack_thread;

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

static void * xstack_thread(void * arg)
{
    while (1) {
        struct ether_hdr hdr;
        int retval;

        printf("Waiting for rx\n");
        retval = ether_receive(ether_handle, &hdr, rx_buffer,
                               sizeof(rx_buffer));
        if (retval == -1) {
            perror("Rx failed");
        } else if (retval > 0) {
            printf("Frame received!\n");
            retval = ether_input(&hdr, rx_buffer, retval);
            if (retval == -1) {
                perror("Protocol handling failed");
            } else if (retval > 0) {
                retval = ether_output_reply(ether_handle, &hdr, rx_buffer,
                                            retval);
                if (retval < 0) {
                    perror("Reply failed");
                }
            }
        }

        if (eval_timer()) {
            printf("tick\n");
            ip_run_periodic_tasks(delta_time);
        }
        if (stop_xstack_thread) {
            break;
        }
    }

    pthread_exit(NULL);
}

int xstack_start(int handle)
{
    ether_handle = handle;

    if (xstack_thread_id) {
        errno = EALREADY;
        return -1;
    }
    return pthread_create(&xstack_thread_id, NULL, xstack_thread, NULL);
}

void xstack_stop(void)
{
    stop_xstack_thread = !0;
    pthread_join(xstack_thread_id, NULL);
}
