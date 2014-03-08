/* Prototypes and definitions for network drivers. */

#ifndef _MINIX_NETDRIVER_H
#define _MINIX_NETDRIVER_H

#include <minix/endpoint.h>
#include <minix/ipc.h>

/* Functions defined by netdriver.c: */
_PROTOTYPE( void netdriver_announce, (void) );
_PROTOTYPE( int netdriver_receive, (endpoint_t src, message *m_ptr,
	int *status_ptr) );

#endif /* _MINIX_NETDRIVER_H */
