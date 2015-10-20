#ifndef XSTACK_IP_H
#define XSTACK_IP_H

#include "xstack_in.h"
#include "linker_set.h"

#define IP_STR_LEN  17

int ip_local_ether_handle;
in_addr_t ip_local_addr;

typedef void ip_periodic_task_t(int delta_time);

#define IP_PERIODIC_TASK(_task_fn_) \
    DATA_SET(_ip_periodic_tasks, _task_fn_)

int ip_config(int ether_handle, in_addr_t ip_addr);
void ip2str(in_addr_t ip, char * buf);
void ip_run_periodic_tasks(int delta_time);

#endif /* XSTACK_IP_H */
