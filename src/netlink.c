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

#include "events.h"


/**
* @author rferreira
* @short Create netlink socket
*
* Creates and bind()s a netlink socket. On error errno will be set.
*
* @param filter groups filter(ex: RTMGRP_IPV4_MROUTE | RTMGRP_IPV6_IFADDR )
* @return -1 on error, otherwise the socket file descriptor is return
*/
int setup_rtsocket(int filter)
{
	int sknl;
	struct sockaddr_nl skaddr;

	sknl = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (sknl < 0) {
		return -1;
	}

	memset(&skaddr, 0, sizeof(struct sockaddr_nl));
	skaddr.nl_family = AF_NETLINK;
	skaddr.nl_groups = filter;

	if (bind(sknl, (struct sockaddr *) &skaddr, sizeof(skaddr)) < 0) {
		return -1;
	}

	return sknl;
}




/**
* @author rferreira
*
* @short rtnetlink messages handler loop
* @param sknl Fully initialized netlink socket
* @see setup_rtsocket
* @return 0 on success, -1 otherwise
*
*/
int loop_rthandle(struct event_handler *h, int sknl)
{
	int bytes, count = 100;
	char buf[2048];
	struct nlmsghdr *nlh;

	while (1) {
		memset(buf, 0, 2048);
		bytes = recv(sknl, buf, 2048, 0);

		if (bytes <= 0) {
			return -1;
		}

		#if 0
		tprintf("Received %d bytes.\n", bytes);
		#endif

		nlh = (struct nlmsghdr *) buf;

		if (NLMSG_OK(nlh, bytes)) {
			event_push(h, nlh, bytes);
			//parse_rt_event(nlh, bytes);
		}
		count--;
	}

	return 0;
}

