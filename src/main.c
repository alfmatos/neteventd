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
#include <net/ethernet.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/neighbour.h>

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
	int type;

	memset(tb, 0, sizeof(struct rtattr *) * max);

	for (rta = data; RTA_OK(rta, len); rta = RTA_NEXT(rta, len)){
		type = rta->rta_type;
		if (type > 0 && type < max) {
			tb[type] = rta;
			atts++;
		}
	}
	
	if (len > 0)
		printf("Unparsed bytes in the RTA\n");

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

void
print_addr_event(void * addr, int family, int ifindex, int event)
{
	char str[INET6_ADDRSTRLEN];
	char ifname[IFNAMSIZ];
	if_indextoname(ifindex, ifname);

	inet_ntop(family, addr, str, INET6_ADDRSTRLEN);

	if (event == RTM_NEWADDR) {
		tprintf("Added %s to dev %s\n", str, ifname);
	}

	if (event == RTM_DELADDR) {
		tprintf("Removed %s from dev %s\n", str, ifname);
	}
}

int
handle_addr_attrs(struct ifaddrmsg * ifa_msg , struct rtattr * tb[], int type)
{
	struct ifa_cacheinfo * cinfo;

	if (tb[IFA_CACHEINFO]) {
		cinfo = RTA_DATA(tb[IFA_CACHEINFO]);
	}

	if (tb[IFA_ADDRESS]) {
		int family = ifa_msg->ifa_family;
		if ( (ifa_msg->ifa_family == AF_INET6)
			|| (ifa_msg->ifa_family == AF_INET) ) {
			if ( cinfo->tstamp == cinfo->cstamp) {
				print_addr_event(
				RTA_DATA(tb[IFA_ADDRESS]),
				family,
				ifa_msg->ifa_index,
				type);
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

void
print_neigh_attrs (struct ndmsg * ndm, void * addr, void * lladdr,
								char * action)
{
	char addr_str[INET6_ADDRSTRLEN], ll_str[INET6_ADDRSTRLEN];

	if (addr)
		inet_ntop(ndm->ndm_family, addr, addr_str, INET6_ADDRSTRLEN);

	if (lladdr)
		ether_ntoa_r(lladdr, ll_str);
		
	tprintf("Neighbour %s:", action);

	if (addr)
		printf(" IP Address(%s)", addr_str);

	if (lladdr)
		printf(" LL address(%s)", ll_str);

	printf("\n");
}

void handle_neigh_attrs(struct ndmsg * ndm, struct rtattr * tb[], int type)
{
	char ifname[IFNAMSIZ];
	void * addr = NULL, * lladdr;

	if_indextoname(ndm->ndm_ifindex, ifname);


	if (tb[NDA_DST]) {
		addr = RTA_DATA(tb[NDA_DST]);	
	}

	if (tb[NDA_LLADDR]) {
		lladdr = RTA_DATA(tb[NDA_LLADDR]);
	}

	if (type == RTM_NEWNEIGH) 
		print_neigh_attrs(ndm, addr, lladdr, "Add Event");
	else if (type == RTM_DELNEIGH)
		print_neigh_attrs(ndm, addr, lladdr, "Del Event");
}

int handle_neigh_msg(struct nlmsghdr * nlh, int n)
{
	struct rtattr * tb[NDA_MAX];
	struct ndmsg * ndm = NLMSG_DATA(nlh);

	parse_rt_attrs(tb, NDA_MAX, RTM_RTA(ndm), RTM_PAYLOAD(nlh));
	handle_neigh_attrs(ndm, tb, nlh->nlmsg_type);

	return 0;
}

void
print_route_attrs(void * src, void * dst, void * gw, int * iif, int * oif,
			struct rtmsg * rtm, char * action)
{
	char gw_str[INET6_ADDRSTRLEN], dst_str[INET6_ADDRSTRLEN];
	char src_str[INET6_ADDRSTRLEN], oif_str[IFNAMSIZ], iif_str[IFNAMSIZ];


	if (dst)
		inet_ntop(rtm->rtm_family, dst, dst_str, INET6_ADDRSTRLEN);

	if (src)
		inet_ntop(rtm->rtm_family, src, src_str, INET6_ADDRSTRLEN);

	if (gw)
		inet_ntop(rtm->rtm_family, gw, gw_str, INET6_ADDRSTRLEN);

	if (oif)
		if_indextoname(*oif, oif_str);

	if (iif)
		if_indextoname(*iif, iif_str);

	if(dst && src && oif && gw) {
		tprintf("%s route %s/%d from %s/%d on dev %s via %s\n",
				action,	dst_str, rtm->rtm_dst_len,
					src_str, rtm->rtm_src_len,
					 oif_str, gw_str);
	} else if( dst && oif && gw) {
		tprintf("%s route %s/%d on dev %s via %s\n",
				action,	dst_str, rtm->rtm_dst_len,
					 oif_str, gw_str);
	} else if (dst && oif) {
		tprintf("%s route %s/%d on dev %s\n",
				action,	dst_str, rtm->rtm_dst_len, oif_str);
	} else if (gw && oif) {
		tprintf("%s default route via %s on dev %s\n",
				action, gw_str, oif_str);
	} else {
		tprintf("%s unknown route type:", action);
		if (gw) printf(" gw");
		if (dst) printf(" dst");
		if (src) printf(" src");
		if (oif) printf(" oif");
		if (iif) printf(" iif");
		printf("\n");
	}
}

int
handle_route_attrs(struct rtmsg * rtm , struct rtattr * tb[], int type)
{
	void * src=NULL, * dst=NULL, * gw=NULL;
	int * iif=NULL, * oif=NULL; 

	if (tb[RTA_DST]) {
		dst = RTA_DATA(tb[RTA_DST]);
	}

	if (tb[RTA_SRC]) {
		src = RTA_DATA(tb[RTA_SRC]);
	}
	
	if (tb[RTA_IIF]) {
		iif = RTA_DATA(tb[RTA_IIF]);
	}

	if (tb[RTA_OIF]) {
		oif = RTA_DATA(tb[RTA_OIF]);
	}

	if (tb[RTA_GATEWAY]) {
		gw = RTA_DATA(tb[RTA_GATEWAY]);
	}

	if (type == RTM_NEWROUTE) {
		print_route_attrs(src, dst, gw, iif, oif, rtm, "Added");
	} else if ( type == RTM_DELROUTE) {
		print_route_attrs(src, dst, gw, iif, oif, rtm, "Removed");
	}

	return 0;
}

void
parse_route_proto(struct rtmsg * rtm)
{
	switch(rtm->rtm_protocol) {
		case RTPROT_KERNEL:
			printf("route added by kernel\n");
			break;
		case RTPROT_BOOT:
			printf("route added at boot timel\n");
			break;
		case RTPROT_STATIC:
			printf("route mannualy added\n");
			break;
		case RTPROT_RA:
			printf("---> route added by RA/RS\n");
			break;
		case RTPROT_ZEBRA:
			printf("route added by Zebra routing daemon\n");
			break;
		default:
			break;
	}
}

int handle_route_msg(struct nlmsghdr * nlh, int n)
{
	struct rtmsg * rtm = NLMSG_DATA(nlh); 
	struct rtattr * tb[RTN_MAX];

	/* parse_route_proto(rtm); */
	parse_rt_attrs(tb, RTN_MAX, RTM_RTA(rtm), RTM_PAYLOAD(nlh));
	handle_route_attrs(rtm, tb, nlh->nlmsg_type);

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
	switch(nlh->nlmsg_type) {
	case RTM_NEWLINK: case RTM_DELLINK: case RTM_GETLINK:
		handle_link_msg(nlh, n);
                break;
	case RTM_NEWADDR: case RTM_DELADDR: case RTM_GETADDR:
		handle_addr_msg(nlh, n); 
		break;
        case RTM_NEWNEIGH: case RTM_DELNEIGH: case RTM_GETNEIGH:
		handle_neigh_msg(nlh, n);
		break;
	case RTM_NEWROUTE: case RTM_DELROUTE: case RTM_GETROUTE:
		handle_route_msg(nlh, n);
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
	skaddr.nl_groups = RTMGRP_LINK
			| RTMGRP_NOTIFY
			| RTMGRP_NEIGH
			| RTMGRP_IPV6_IFADDR
			| RTMGRP_IPV6_ROUTE
			| RTMGRP_IPV6_MROUTE
			| RTMGRP_IPV6_IFINFO
			| RTMGRP_IPV4_IFADDR 
			| RTMGRP_IPV4_ROUTE
			| RTMGRP_IPV4_MROUTE;

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
