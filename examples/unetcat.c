#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xstack_socket.h"

static char buf[2048];

int main(void)
{
    void * sock;

    sock = xstack_listen("/tmp/unetcat.sock");
    if (!sock) {
        perror("Failed to open sock");
        exit(1);
    }

    while (1) {
        struct xstack_sockaddr addr;
        size_t r;

        memset(buf, 0, sizeof(buf));
        r = xstack_recvfrom(sock, buf, sizeof(buf) - 1, 0, &addr);
        if (r > 0)
            write(STDOUT_FILENO, buf, r);
    }
}
