/**
 * @addtogroup XSTACK_CONFIG
 * @{
 */

#ifndef CONFIG_H
#define CONFIG_H

/**
 * Disable or enable logging.
 */
#define XSTACK_LOGGING              1

#define XSTACK_DATAGRAM_SIZE_MAX    1024

#define XSTACK_DATAGRAM_BUF_SIZE    2048

/**
 * Periodic IP event tick.
 * How often should periodic tasks run.
 * This is handled by IP but it's meant to be more generic.
 */
#define XSTACK_PERIODIC_EVENT_SEC   10

/**
 * ARP Configuration.
 * @{
 */

/**
 * ARP Cache size.
 * The size of ARP cache in entries.
 * If ARP runs out of slots it will free the oldest validdynamic entry in
 * the cache; if all entries all static and thus there is no more empty
 * slots left the ARP insert will fail.
 */
#define XSTACK_ARP_CACHE_SIZE       50

/**
 * @}
 */

/*
 * @{
 * IP Configuration.
 */

/**
 * RIB (Routing Information Base) size in the number of entries.
 */
#define XSTACK_IP_RIB_SIZE          5

/**
 * Max number of deferred IP packets.
 * Maximum number of IP packets waiting for transmission, ie. waiting for ARP
 * to provide a destination MAC address.
 */
#define XSTACK_IP_DEFER_MAX         20

/**
 * Unreachable destination IP.
 * + 0 = Drop silently
 * + 1 = Send ICMP Destination host unreachable
 */
#define XSTACK_IP_SEND_HOSTUNREAC   1

/**
 * The number of buffers reserved for IP fragment reassembly.
 */
#define XSTACK_IP_FRAGMENT_BUF      4

/**
 * IP fragment reassembly timer lower bound [sec].
 * The RFC recommends a default value of 15 seconds.
 */
#define XSTACK_IP_FRAGMENT_TLB      15

/**
 * @}
 */

#endif /* CONFIG_H */

/**
 * @}
 */
