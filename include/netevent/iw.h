#ifndef __NETEVENT_IW__
#define __NETEVENT_IW__

#include <iwlib.h>

int handle_wireless_attr(int ifindex, char * data, int len);
int handle_wireless_event(int ifindex, struct iw_event * iwe);

#endif
