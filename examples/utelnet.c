#include <stdio.h>
#include <stdlib.h>

#include "xstack_socket.h"

char buf[80];

int main(void)
{
    void * sock;

    sock = xstack_listen("/tmp/utelnet.sock");
    if (!sock) {
        perror("Failed to open sock");
        exit(1);
    }

    while (1) {
        struct xstack_sockaddr addr;

        xstack_recvfrom(sock, buf, sizeof(buf), 0, &addr);
        fprintf(stderr, buf);
    }
}
