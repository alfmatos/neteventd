#ifndef __NETEVENT_NETLINK__
#define __NETEVENT_NETLINK__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <signal.h>

#include <asm/types.h>
#include <arpa/inet.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/ethernet.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <netevent/events.h>

/**
* @author rferreira
* @short Create netlink socket
*
* Creates and bind()s a netlink socket. On error errno will be set.
*
* @param filter groups filter(ex: RTMGRP_IPV4_MROUTE | RTMGRP_IPV6_IFADDR )
* @return -1 on error, otherwise the socket file descriptor is return
*/
int setup_rtsocket(int filter);



/**
* @author rferreira
*
* @short rtnetlink messages handler loop
* @param sknl Fully initialized netlink socket
* @see setup_rtsocket
* @return 0 on success, -1 otherwise
*
*/
int loop_rthandle(struct event_handler *h, int sknl);


#endif