/*
 * btstack bluetooth serial port profile driver
 */

#include <stdint.h>

#ifndef _SPP_H_
#define _SPP_H_

int spp_init(void);
int spp_printf(const char *format, ...);
char *spp_recv(int *size);

#endif /* _SPP_H_ */
