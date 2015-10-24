/**
 * @addtogroup ICMP
 * @{
 */

#ifndef XSTACK_ICMP_H
#define XSTACK_ICMP_H

#include <stdint.h>

/**
 * ICMP message.
 */
struct icmp {
    uint8_t icmp_type;
    uint8_t icmp_code;
    uint16_t icmp_csum;
    uint32_t icmp_rest;
    uint8_t icmp_data[0];
};

/**
 * ICMP Types.
 * @{
 */
#define ICMP_TYPE_ECHO_REPLY    0
#define ICMP_TYPE_DESTUNREAC    3
#define ICMP_TYPE_ECHO          8
/**
 * @}
 */

/**
 * ICMP Codes for ICMP_TYPE_DESTUNREAC.
 * @{
 */
#define ICMP_CODE_DESTUNREAC    0
#define ICMP_CODE_HOSTUNREAC    1
#define ICMP_CODE_PROTOUNREAC   2
#define ICMP_CODE_PORTUNREAC    3
#define ICMP_CODE_DESTNETUNK    6
#define ICMP_CODE_HOSTUNK       7
/**
 * @}
 */

#endif /* XSTACK_ICMP_H */

/**
 * @}
 */
