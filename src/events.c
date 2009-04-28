#include <errno.h>
#include <netevent/events.h>




void event_init(struct event_handler *h)
{
	int i;

	for ( i=0; i<MAX_HANDLERS; i++ ) {
		h->async[i] = NULL;
		h->sync[i] = NULL;
	}
}


int event_register(struct event_handler *h, ev_handler_t fun)
{
	int i;
	for ( i=0; i<MAX_HANDLERS && h->sync[i] ; i++)
		;;

	if ( i == MAX_HANDLERS ) {
		errno = ENOMEM;
		return -1;
	}

	h->sync[i] = fun;

	return 0;
}


void event_push(struct event_handler *h, void *buf, size_t len)
{
	int i;

	for ( i=0; i<MAX_HANDLERS; i++) {
		if ( h->sync[i] != NULL ) {
			h->sync[i](buf, len);
		}
	}
}



