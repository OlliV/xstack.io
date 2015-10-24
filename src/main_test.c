#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "xstack_ether.h"
#include "xstack_ip.h"

static uint8_t rx_buffer[ETHER_MAXLEN];

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

int main(void)
{
    char * const ether_args[] = {
        "enp0s20u1",
        NULL,
    };
    int handle;

    handle = ether_init(ether_args);
    if (handle == -1) {
        perror("Failed to init");
        exit(1);
    }

    if (ip_config(handle, 167772161)) {
        perror("Failed to config IP");
        exit(1);
    }

    ip_send(167772161, 167772162, IP_PROTO_SCTP,
            (uint8_t *)"test", 5);

    while (1) {
        struct ether_hdr hdr;
        int retval;

        printf("Waiting for rx\n");
        retval = ether_receive(handle, &hdr, rx_buffer, sizeof(rx_buffer));
        if (retval == -1) {
            perror("Rx failed");
        } else if (retval > 0) {
            printf("Frame received!\n");
            retval = ether_input(&hdr, rx_buffer, retval);
            if (retval == -1) {
                perror("Protocol handling failed");
            } else if (retval > 0) {
                retval = ether_output_reply(handle, &hdr, rx_buffer, retval);
                if (retval < 0) {
                    perror("Reply failed");
                }
            }
        }

        if (eval_timer()) {
            printf("tick\n");
            ip_run_periodic_tasks(delta_time);
        }
    }

    ether_deinit(handle);

    return 0;
}
