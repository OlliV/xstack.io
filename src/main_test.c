#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "xstack_ether.h"
#include "xstack_ip.h"
#include "xstack_socket.h"

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

void * socket_test_thread(void * arg)
{
    struct xstack_sock * sock = (struct xstack_sock *)arg;
    struct xstack_sockaddr sockaddr;
    char buf[1600];

    while (1) {
        int retval;

        retval = xstack_recvfrom(sock, buf, sizeof(buf), 0, &sockaddr);
        if (retval == -1) {
            static int count;

            if (count++ < 3) {
                perror("Failed to receive");
            }
        } else if (retval >= 0) {
            char ipstr[IP_STR_LEN];

            ip2str(sockaddr.inet4_addr, ipstr);
            printf("Received %d bytes over UDP from %s\n", retval, ipstr);
        }
    }

    pthread_exit(NULL);
}

void socket_test(void)
{
    static struct xstack_sock * sock;
    struct xstack_sockaddr sockaddr = {
        .inet4_addr = 167772162,
        .port = 10,
    };
    pthread_t thread;

    sock = xstack_socket(XF_INET4, XSOCK_DGRAM, XIP_PROTO_UDP);
    if (!sock) {
        perror("Failed to get a socket");
        exit(1);
    }
    if (xstack_bind(sock, sockaddr) < 0) {
        perror("Failed to bind a socket");
        exit(1);
    }

    if (pthread_create(&thread, NULL, socket_test_thread, sock)) {
        perror("Failed to create a thread");
        exit(1);
    }
}

int main(void)
{
    char * const ether_args[] = {
        //"enp0s20u1",
        "veth1",
        NULL,
    };
    int handle;

    handle = ether_init(ether_args);
    if (handle == -1) {
        perror("Failed to init");
        exit(1);
    }

    if (ip_config(handle, 167772162, 4294967040)) {
        perror("Failed to config IP");
        exit(1);
    }

    socket_test();

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

            /* Testing */
            if (ip_send(167772161, IP_PROTO_SCTP, (uint8_t *)"test", 5) < 0) {
                perror("Failed to send");
            }
        }
    }

    ether_deinit(handle);

    return 0;
}
