#ifndef CONFIG_H
#define CONFIG_H

/**
 * Periodic IP event tick.
 * How often should periodic tasks run.
 * This is handled by IP but it's meant to be more generic.
 */
#define XSTACK_PERIODIC_EVENT_SEC   10

/*
 * @{
 * IP Configuration.
 */

/**
 * Unreachable destination IP.
 * 0 = Drop silently
 * 1 = Send ICMP Destination host unreachable.
 */
#define XSTACK_IP_SEND_HOSTUNREAC   1

/**
 * @}
 */

#endif /* CONFIG_H */
