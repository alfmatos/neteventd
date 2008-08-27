#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include <time.h> 

#include <asm/types.h>
#include <arpa/inet.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <net/if.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "config.h"

int
tprintf(char * format, ...)
{
	va_list argp;
	char msg[2048], timestamp[20];
	struct timeval tv;
	struct tm * t;

	gettimeofday(&tv, NULL);
	t = localtime(&tv.tv_sec);

	sprintf(timestamp, "[%02d:%02d:%02d.%06ld]",
		t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec);

	va_start(argp, format);
	vsnprintf(msg, sizeof(msg), format, argp);
	va_end(argp);

	printf("%s %s", timestamp, msg);
	return 0;
}

void
short_header()
{
	printf("%s - Copyright (C) 2008 Alfredo Matos.\n", PACKAGE_STRING);
}

void
long_header()
{
	printf("%s\n", PACKAGE_STRING);
	printf("Copyright (C) 2008 Alfredo Matos.\n"
	"License GPLv3+:"
	" GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
	"This is free software: you are free to change and redistribute it.\n"
	"There is NO WARRANTY, to the extent permitted by law.\n");
}


int debug_rt_msg(struct rtmsg * rti)
{
	char * strt;


	switch(rti->rtm_type) {
	case RTN_UNICAST:
		break;
	case RTN_LOCAL:
		break;
	case RTN_MULTICAST:
		break;
	case RTN_BROADCAST:
		break;
	default:
		break;

	}

	return 0;
}

int parse_rt_attrs(struct rtattr * tb[], int max, struct rtattr * data, int len)
{

	struct rtattr * rta;
	int atts=0;

	memset(tb, 0, sizeof(struct rtattr *) * max);

	for (rta = data; RTA_OK(rta, len); rta = RTA_NEXT(rta, len)){
		if ( rta->rta_type < max) {
			tb[rta->rta_type] = rta;
			atts++;
		}
	}

	return atts;
}

int parse_ifinfomsg(struct ifinfomsg * msg)
{
	char ifname[IFNAMSIZ];
	if_indextoname(msg->ifi_index, ifname);

	if (msg->ifi_change & IFF_UP && (msg->ifi_flags & IFF_UP) ) {
		tprintf("Interface %s changed to UP\n", ifname);
	}

	if (msg->ifi_change & IFF_UP && !(msg->ifi_flags & IFF_UP) ) {
		tprintf("Interface %s changed to DOWN\n", ifname);
	}


	return 0;
}

int handle_addr_add_msg(struct ifaddrmsg * ifa_msg)
{
	return 0;
}

int
handle_addr_attrs(struct ifaddrmsg * ifa_msg , struct rtattr * tb[], int type)
{
	struct in6_addr * addr6;
	struct in_addr * addr4;
	char str[INET6_ADDRSTRLEN];

	char ifname[IFNAMSIZ];
	if_indextoname(ifa_msg->ifa_index, ifname);

	if (tb[IFA_ADDRESS]) {
		if (ifa_msg->ifa_family == AF_INET6) {
			addr6 = (struct in6_addr*) RTA_DATA(tb[IFA_ADDRESS]);
			inet_ntop(AF_INET6, addr6, str, INET6_ADDRSTRLEN);
			if (type == RTM_NEWADDR) {
				tprintf("Added %s to dev %s\n", str, ifname);
			}
			if (type == RTM_DELADDR) {
				tprintf("Removed %s from dev %s\n",
								str, ifname);
			}
		}

		if (ifa_msg->ifa_family == AF_INET) {
			addr4 = (struct in_addr*) RTA_DATA(tb[IFA_ADDRESS]);
			inet_ntop(AF_INET, addr4, str, INET6_ADDRSTRLEN);
			if (type == RTM_NEWADDR) {
				tprintf("Added %s to dev %s\n", str, ifname);
			}
			if (type == RTM_DELADDR) {
				tprintf("Removed %s from dev %s\n",
								str, ifname);
			}
		}
	}

	if (tb[IFA_LOCAL]) {
	}

	if (tb[IFA_LABEL]) {
	}

	if (tb[IFA_BROADCAST]) {
	}

	if (tb[IFA_ANYCAST]) {
	}

	if (tb[IFA_CACHEINFO]) {
	}

	return 0;
}

int handle_addr_msg(struct nlmsghdr * nlh, int n)
{
	struct ifaddrmsg * ifa_msg = NLMSG_DATA(nlh); 
	struct rtattr * tb[IFA_MAX];


	parse_rt_attrs(tb, IFA_MAX, IFA_RTA(ifa_msg), IFA_PAYLOAD(nlh));
	handle_addr_attrs(ifa_msg, tb, nlh->nlmsg_type);
	return 0;
}

int handle_neigh_msg(struct nlmsghdr * nlh, int n)
{
	/*struct rtattr * tb[IFA_MAX]; */
	struct ndmsg * ndi;
	char ifname[IFNAMSIZ];
	ndi = (struct ndmsg *) NLMSG_DATA(nlh);
	if_indextoname(ndi->ndm_ifindex, ifname);

/*	parse_rt_attrs(tb, IFA_MAX, IFA_RTA(ifa_msg), IFA_PAYLOAD(nlh)); */

	switch(nlh->nlmsg_type) {
        case RTM_NEWNEIGH:
		tprintf("Neibhbour Add Event on device (%s)\n", ifname);
                break;
        case RTM_DELNEIGH:
		tprintf("Neighbor Del Event on device (%s)\n", ifname);
                break;
	case RTM_GETNEIGH:
		tprintf("Neighbor GET event\n");
		break;
	}

	return 0;
}

int handle_link_msg(struct nlmsghdr * nlh, int n)
{
	struct ifinfomsg * ifla_msg = NLMSG_DATA(nlh); 
	struct rtattr * tb[IFLA_MAX];
	char ifname[IFNAMSIZ];

	parse_rt_attrs(tb, IFLA_MAX, IFLA_RTA(ifla_msg), IFLA_PAYLOAD(nlh));
	if_indextoname(ifla_msg->ifi_index, ifname);

	parse_ifinfomsg(ifla_msg); 

	switch(nlh->nlmsg_type) {
	case RTM_NEWLINK:
		break;
	case RTM_DELLINK:
		break;
	case RTM_GETLINK:
		break;
	default:
		break;
	} 

	return 0;
}

int parse_rt_event(struct nlmsghdr * nlh, int n)
{
	struct rtmsg * rti;
	char ifname[IFNAMSIZ];

	switch(nlh->nlmsg_type) {
	case RTM_NEWLINK: case RTM_DELLINK: case RTM_GETLINK:
		handle_link_msg(nlh, n);
                break;
	case RTM_NEWADDR: case RTM_DELADDR: case RTM_GETADDR:
		handle_addr_msg(nlh, n); 
		break;
        case RTM_NEWNEIGH: case RTM_DELNEIGH: case RTM_GETNEIGH:
		break;
	case RTM_NEWROUTE: case RTM_DELROUTE: case RTM_GETROUTE:
		rti = (struct rtmsg *) NLMSG_DATA(nlh);
		debug_rt_msg(rti);
		break;
        default:
                tprintf("Unknown netlink event\n");
                break;
        }

	return 0;
}

int main(void)
{
	int sknl, bytes, count=100;
	struct sockaddr_nl skaddr;
	char buf[2048];
	char ifname[IFNAMSIZ];
	struct nlmsghdr * nlh; 
	struct ifinfomsg * ifmsg;

	short_header();

	sknl = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	if ( sknl < 0 ) {
		printf("Error %d: %s\n", errno, strerror(errno));
		exit(1);
	}

	memset(&skaddr, 0, sizeof(struct sockaddr_nl)); 
	skaddr.nl_family = AF_NETLINK;
	skaddr.nl_groups = 
	( RTNLGRP_LINK | RTNLGRP_NOTIFY | RTNLGRP_NEIGH
	| RTMGRP_IPV6_IFADDR | RTMGRP_IPV6_MROUTE | RTMGRP_IPV6_ROUTE | RTMGRP_IPV6_IFINFO
	| RTMGRP_IPV4_IFADDR
	| RTNLGRP_IPV6_IFADDR | RTNLGRP_IPV6_MROUTE | RTNLGRP_IPV6_ROUTE
	| RTNLGRP_IPV4_IFADDR | RTNLGRP_IPV4_MROUTE | RTNLGRP_IPV4_ROUTE );

	if( bind(sknl, (struct sockaddr *) &skaddr, sizeof(skaddr)) < 0 ) {
		printf("Error %d: %s\n", errno, strerror(errno));
		exit(1);
	}

	while (1)	{
		memset(buf, 0, 2048);
		bytes = recv(sknl, buf, 2048, 0);

		if ( bytes <= 0 ) {
			printf("Error %d: %s\n", errno, strerror(errno));
			exit(1);
		}

#if 0
		tprintf("Received %d bytes.\n", bytes);
#endif

		nlh = (struct nlmsghdr *) buf;

		if (NLMSG_OK(nlh, bytes)) {
			parse_rt_event(nlh, bytes);
		}
		count--;
	}

	close(sknl);
	return 0;
}
