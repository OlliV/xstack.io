#ifndef XSTACK_IN_H
#define XSTACK_IN_H

#include <arpa/inet.h> /* TODO Maybe we want to define our own version */
#include <stdint.h>

typedef uint32_t in_addr_t;
typedef uint16_t in_port_t;

/**
 * Convert an IP address from integer representation to a C string.
 * @note The minimum size of buf is IP_STR_LEN.
 * @param[in] ip is the IP address to be converted.
 * @param[out] buf is the destination buffer.
 */
void ip2str(in_addr_t ip, char * buf);

#endif /* XSTACK_IN_H */
