#include <stdio.h>
#include "xstack_arp.h"
#include "xstack_in.h"
#include "xstack_ip.h"

SET_DECLARE(_ip_periodic_tasks, void);

int ip_local_ether_handle;
in_addr_t ip_local_addr;

/* TODO Mask */
int ip_config(int ether_handle, in_addr_t ip_addr)
{
    ip_local_ether_handle = ether_handle;
    ip_local_addr = ip_addr;

    for (size_t i = 0; i < 5; i++) {
        arp_gratuitous(ether_handle, ip_local_addr);
    }

    return 0;
}

void ip2str(in_addr_t ip, char * buf)
{
    unsigned char bytes[4];

    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
    sprintf(buf, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
}

void ip_run_periodic_tasks(int delta_time)
{
    void ** taskp;

    SET_FOREACH(taskp, _ip_periodic_tasks) {
        ip_periodic_task_t * task = *(ip_periodic_task_t **)taskp;

        if (task)
            task(delta_time);
    }
}
