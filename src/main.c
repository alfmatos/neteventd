#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/time.h>
#include <time.h> 

#include <asm/types.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <net/if.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "config.h"

int debug_rt_msg(struct rtmsg * rti)
{
	char * strt;


	switch(rti->rtm_type) {
	case RTN_UNICAST:
		printf("Unicast Route\n");
		break;
	case RTN_LOCAL:
		printf("Local Route\n");
		break;
	case RTN_MULTICAST:
		printf("Multicast Route\n");
		break;
	case RTN_BROADCAST:
		printf("Broadcast Route\n");
		break;
	default:
		break;

	}

	return 0;
}

int parse_rt_atts(struct rtattr * tb[]);

int parse_rt_event(struct nlmsghdr * nlh, int n)
{
	char ifname[IFNAMSIZ];
	struct ifinfomsg * ifmsg;
	struct ndmsg * ndi;
	struct rtmsg * rti;

	switch(nlh->nlmsg_type) {
	case RTM_NEWLINK:
		ifmsg = (struct ifinfomsg *) NLMSG_DATA(nlh);
		if_indextoname(ifmsg->ifi_index, ifname);
		printf("Link Up Event on device (%s)\n",
					ifname, ifmsg->ifi_index );
		break;
	case RTM_DELLINK:
		ifmsg = (struct ifinfomsg *) NLMSG_DATA(nlh);
		if_indextoname(ifmsg->ifi_index, ifname);
		printf("Link Down Event on device (%s)\n",
					ifname, ifmsg->ifi_index );
		break;
	case RTM_GETLINK:
                break;
	case RTM_NEWADDR:
		printf("Address Add Event\n");
		break;
	case RTM_DELADDR:
		printf("Address Del Event\n");
		break;
	case RTM_GETADDR:
		printf("Address Event\n");
		break;
        case RTM_NEWNEIGH:
		ndi = (struct ndmsg *) NLMSG_DATA(nlh);
		if_indextoname(ndi->ndm_ifindex, ifname);
		printf("Neibhbour Add Event on device (%s)\n",
						ifname, ndi->ndm_ifindex );
                break;
        case RTM_DELNEIGH:
		ndi = (struct ndmsg *) NLMSG_DATA(nlh);
		if_indextoname(ndi->ndm_ifindex, ifname);
		printf("Neighbor Del Event on device (%s)\n", ifname, ndi->ndm_ifindex );
                break;
	case RTM_GETNEIGH:
		printf("Neighbor GET event\n");
		break;
	case RTM_NEWROUTE:
		printf("Route Add event\n");
		rti = (struct rtmsg *) NLMSG_DATA(nlh);
		debug_rt_msg(rti);
		break;
	case RTM_DELROUTE:
		printf("Route Del event\n");
		rti = (struct rtmsg *) NLMSG_DATA(nlh);
		debug_rt_msg(rti);
		break;
	case RTM_GETROUTE:
		printf("Route GET event\n");
		rti = (struct rtmsg *) NLMSG_DATA(nlh);
		debug_rt_msg(rti);
		break;

        default:
                printf("Unknown netlink event\n");
                break;
        }
}

int main(void)
{
	int sknl, bytes, count=100;
	struct sockaddr_nl skaddr;
	char buf[2048];
	char ifname[IFNAMSIZ];
	struct nlmsghdr * nlh; 
	struct ifinfomsg * ifmsg;
	struct rtattr * attr, *attr2;

	struct timeval tv;
	struct tm * t;

	printf("%s Copyright (C) 2008 Alfredo Matos\n", PACKAGE_STRING);

	sknl = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	if ( sknl < 0 ) {
		printf("Error %d: %s\n", errno, strerror(errno));
		exit(1);
	}

	memset(&skaddr, 0, sizeof(struct sockaddr_nl)); 
	skaddr.nl_family = AF_NETLINK;
	skaddr.nl_groups = RTNLGRP_LINK | RTNLGRP_NOTIFY | RTNLGRP_NEIGH 
			| RTNLGRP_IPV6_IFADDR
			| RTNLGRP_IPV6_MROUTE
			| RTNLGRP_IPV6_ROUTE
			| RTNLGRP_IPV4_IFADDR
			| RTNLGRP_IPV4_MROUTE
			| RTNLGRP_IPV4_ROUTE;

	if( bind(sknl, (struct sockaddr *) &skaddr, sizeof(skaddr)) < 0 ) {
		printf("Error %d: %s\n", errno, strerror(errno));
		exit(1);
	}

	while ( count )	{
		bytes = recv(sknl, buf, 2048, 0);

		if ( bytes <= 0 ) {
			printf("Error %d: %s\n", errno, strerror(errno));
			exit(1);
		}

		gettimeofday(&tv, NULL);

		char * strt = ctime(&tv.tv_sec);

		t = localtime(&tv.tv_sec);

		printf("%s", strt);

		printf("[%02d:%02d:%02d.%06ld] Received %d bytes\n",
			t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec, bytes);

		nlh = (struct nlmsghdr *) buf;

		if (NLMSG_OK(nlh, bytes)) {
			parse_rt_event(nlh, bytes);
		}

		count--;
	}

	close(sknl);
	return 0;
}
