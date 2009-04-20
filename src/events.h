/**
 * @file events.h Data consuption event handlers
 *
 * Some simple methods to keep track and dispatch handler functions
 *
 */

#ifndef __NETEVENT_EVENTS__
#define __NETEVENT_EVENTS__

#include <stdlib.h>
#include <sys/types.h>

typedef int (*ev_handler_t)(void *data, size_t len);



#define MAX_HANDLERS 64

struct event_handler
{
	ev_handler_t async[MAX_HANDLERS]; // TODO: NOT IMPLEMENTED
	ev_handler_t sync[MAX_HANDLERS];
};

/**
* @short Initialize event_handler
*
*/
void event_init(struct event_handler *h);

/**
* @short Register event handler
*
* @return 0 on success. If the handler limit has been reached, errno will be set as ENOMEM
* @see event_init
*/
int event_register(struct event_handler *h, ev_handler_t fun);

/**
* @short Push event to event handlers
* @see event_init
*/
void event_push(struct event_handler *h, void *buf, size_t len);

#endif