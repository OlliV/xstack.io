/**
 * @addtogroup xstack
 * @{
 */

#ifndef XSTACK_H
#define XSTACK_H

/**
 * Start the IP stack.
 * @param handle is a handle to the interface used.
 * @returns Returns 0 if succesfully started;
 *          Otherwise -1 is returned and errno is set.
 */
int xstack_start(int handle);

/**
 * Stop the IP stack.
 */
void xstack_stop(void);

#endif /* XSTACK_H */

/**
 * @}
 */
