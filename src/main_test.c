#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xstack.h"
#include "xstack_ip.h"
#include "xstack_socket.h"

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

            retval = xstack_send(sock, buf, retval, &sockaddr, 0);
            if (retval == -1) {
                perror("Failed to send a response");
            } else {
                printf("Response sent to %s:%d\n", ipstr, sockaddr.port);
            }
        }
    }

    pthread_exit(NULL);
}

static void socket_test_init(void)
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

    //kill(getpid(), SIGSTOP);

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

    socket_test_init();

    xstack_start(handle);

    sigwaitinfo(&sigset, NULL);

    fprintf(stderr, "Stopping the IP stack...\n");

    xstack_stop();

    ether_deinit(handle);

    return 0;
}
