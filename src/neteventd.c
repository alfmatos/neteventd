/*
 *  Copyright (C) 2008 IT Aveiro
 *
 *  Author: Alfredo Matos <alfredo.matos@av.it.pt>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>

#include <asm/types.h>
#include <arpa/inet.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/ethernet.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <iwlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <netevent/netevent.h>

#define BLACK 		0
#define RED 		1
#define GREEN 		2
#define YELLOW 		3
#define BLUE 		4
#define MAGENTA 	5
#define CYAN 		6
#define WHITE 		7
#define NONE		128

#define OPT_UNKNOWN 	0
#define OPT_COLOR 	1

#define DEFAULT_FILTER	(RTMGRP_LINK | RTMGRP_NOTIFY | RTMGRP_NEIGH | RTMGRP_IPV6_IFADDR | RTMGRP_IPV6_ROUTE | RTMGRP_IPV6_MROUTE | RTMGRP_IPV6_IFINFO | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE | RTMGRP_IPV4_MROUTE);


#define tprintf(args...) eprintf(NONE, ##args)

static int color_output=0;

void colorize(char * cmd, int color)
{
	sprintf(cmd, "\e[%dm", color + 30);
}

/**
 * @short Whether or not the terminal is "dumb"(no color support)
 * @return 1 if it is dumb, 0 otherwise
 */
static int is_terminal_dumb()
{
	char *term = getenv("TERM");

	if ( term && strncmp(term, "TERM", 4) == 0 ) {
		return 1;
	}

	return 0;
}

/**
 * @short Enable colored output
 *
 * i.e. it just sets color_output to 1, but does some additional
 * sanity checks
 *
 * @return 0 if color was not enabled
 */
static int enable_color_output(void)
{

	if (isatty( fileno(stdout)) == 0 ) {
		color_output = 0;
	} else if ( is_terminal_dumb() ) {
		color_output = 0;
	}  else {
		color_output = 1;
	}

	if ( color_output == 0 ) {
		fprintf(stderr, "Colored output doesn't seem to be supported by the terminal...disabling\n");
	}

	return color_output;
}

int eprintf(int color, char *format, ...)
{
	va_list argp;
	char msg[2048], timestamp[20];
	struct timeval tv;
	struct tm *t;
	char fg[13]="", fg_reset[]="\e[0m";

	gettimeofday(&tv, NULL);
	t = localtime(&tv.tv_sec);

	sprintf(timestamp,"[%02d:%02d:%02d.%06ld]",
		t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec);

	va_start(argp, format);
	vsnprintf(msg, sizeof(msg), format, argp);
	va_end(argp);

	if(color_output && (color != NONE)) {
		colorize(fg, color);
	}

	printf("%s%s %s%s", fg, timestamp, msg, fg_reset);
	fflush(stdout);

	return 0;
}


/**
 * @short Cleanup before exit
 *
 * - Reset terminal colors
 * - Flush stdout buffer
 */
static void exit_cleanup(void)
{
	fflush(stdout);
	printf("\e[0m");
}

static void signal_handler(int sig)
{
	exit(0);
}

static void short_header()
{
	printf("%s - Copyright (C) 2009 IT Aveiro.\n",PACKAGE_STRING);
}

static void long_header()
{
	printf("%s\n", PACKAGE_STRING);
	printf("Copyright (C) 2009 IT Aveiro.\n"
	       "License GPLv3+: GNU GPL version 3 or later "
	       "<http://gnu.org/licenses/gpl.html>\n"
	       "This is free software: "
	       "you are free to change and redistribute it.\n"
	       "There is NO WARRANTY, to the extent permitted by law.\n");
}

static void usage()
{
	printf("\nUsage: neteventd [OPTIONS] [FILTERS]]\n"
		"Options:\n"
		"\t-c, --color\tcontrol whether color is used\n"
		"\t-h, --help\tdisplay this help and exit\n"
		"\nFilters:\n"
		"\tRTMGRP_LINK RTMGRP_NOTIFY RTMGRP_NEIGH RTMGRP_IPV6_IFADDR\n"
		"\tRTMGRP_IPV6_ROUTE RTMGRP_IPV6_MROUTE RTMGRP_IPV6_IFINFO\n"
		"\tRTMGRP_IPV4_IFADDR RTMGRP_IPV4_ROUTE RTMGRP_IPV4_MROUTE\n"
		);
}

static int parse_rt_attrs(struct rtattr *tb[], int max, struct rtattr *data,
		   int len)
{
	struct rtattr *rta;
	int atts = 0;
	int type;

	memset(tb, 0, sizeof(struct rtattr *) * max);

	for (rta = data; RTA_OK(rta, len); rta = RTA_NEXT(rta, len)) {
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

static int parse_ifinfomsg(struct ifinfomsg *msg)
{
	char ifname[IFNAMSIZ];

	if_indextoname(msg->ifi_index, ifname);

	if (msg->ifi_change & IFF_UP && (msg->ifi_flags & IFF_UP))
		eprintf(GREEN, "Interface %s changed to UP\n", ifname);

	if (msg->ifi_change & IFF_UP && !(msg->ifi_flags & IFF_UP))
		eprintf(RED, "Interface %s changed to DOWN\n", ifname);

	return 0;
}

static void print_addr_event(void *addr, int family, int ifindex, int event)
{
	char str[INET6_ADDRSTRLEN];
	char ifname[IFNAMSIZ];

	if_indextoname(ifindex, ifname);
	inet_ntop(family, addr, str, INET6_ADDRSTRLEN);

	if (event == RTM_NEWADDR)
		eprintf(GREEN, "Added %s to dev %s\n", str, ifname);

	if (event == RTM_DELADDR)
		eprintf(RED, "Removed %s from dev %s\n", str, ifname, RED);
}

static inline valid_family(const int family)
{
	return ((family == AF_INET) || (family == AF_INET6));
}

static inline cache_new_address_ts(const struct ifa_cacheinfo * ci)
{
	return (ci->tstamp == ci->cstamp);
}

static int handle_addr_attrs(const struct ifaddrmsg *ifa_msg, struct rtattr *tb[],
		      int type)
{
	struct ifa_cacheinfo *ci=NULL;
	void * addr=NULL;
	int family, ifindex;

	family = ifa_msg->ifa_family;
	ifindex = ifa_msg->ifa_index;

	if (tb[IFA_CACHEINFO]) {
		ci = RTA_DATA(tb[IFA_CACHEINFO]);
	}

	if (tb[IFA_ADDRESS]) {
		addr = RTA_DATA(tb[IFA_ADDRESS]);
	}

	/* TODO: Parse remaining attributes */
# if 0
	if (tb[IFA_LOCAL]) {
	}
	if (tb[IFA_LABEL]) {
	}
	if (tb[IFA_BROADCAST]) {
	}
	if (tb[IFA_ANYCAST]) {
	}
# endif

	if (ci && addr && valid_family(family) && cache_new_address_ts(ci))
		print_addr_event(addr, family, ifindex, type);

	return 0;
}


static int handle_addr_msg(struct nlmsghdr *nlh, int n)
{
	struct ifaddrmsg *ifa_msg = NLMSG_DATA(nlh);
	struct rtattr *tb[IFA_MAX];

	parse_rt_attrs(tb, IFA_MAX, IFA_RTA(ifa_msg), IFA_PAYLOAD(nlh));
	handle_addr_attrs(ifa_msg, tb, nlh->nlmsg_type);

	return 0;
}

static void print_neigh_attrs(struct ndmsg *ndm, void *addr, void *lladdr,
		       char *action, int color)
{
	char addr_str[INET6_ADDRSTRLEN], ll_str[INET6_ADDRSTRLEN];
	char ifname[IFNAMSIZ], output[2048];
	int len;

	if_indextoname(ndm->ndm_ifindex, ifname);

	if (addr)
		inet_ntop(ndm->ndm_family, addr, addr_str, INET6_ADDRSTRLEN);

	if (lladdr)
		ether_ntoa_r(lladdr, ll_str);

	len = sprintf(output, "%s neighbor on %s:", action, ifname);

	if (addr)
		len += sprintf(output+len, " [%s]", addr_str);

	if (lladdr)
		len += sprintf(output+len," [%s]", ll_str);

	eprintf(color, "%s\n", output);
}

static void parse_ndm_state(uint16_t state, struct nda_cacheinfo *ci)
{
	tprintf("Debug Nud state:");

	if (state & NUD_INCOMPLETE)
		printf(" NUD_INCOMPLETE");

	if (state & NUD_REACHABLE)
		printf(" NUD_REACHABLE");

	if (state & NUD_STALE)
		printf(" NUD_STALE");

	if (state & NUD_DELAY)
		printf(" NUD_DELAY");

	if (state & NUD_FAILED)
		printf(" NUD_FAILED");

	if (state & NUD_NOARP)
		printf(" NUD_NOARP");

	if (state & NUD_PERMANENT)
		printf(" NUD_PERMANENT");


	if (ci)
		printf("Cache: confirmed %d updated %d used %d refcnt %d\n",
			ci->ndm_confirmed, ci->ndm_updated, ci->ndm_used,
			ci->ndm_refcnt);
}

static inline detect_new_neigh(struct ndmsg *ndm, struct nda_cacheinfo *ci,
				  int type)
{
	return ((type == RTM_NEWNEIGH)
		&& (((ndm->ndm_state & NUD_REACHABLE) && (ci->ndm_updated==0)
			&& (ci->ndm_used==0) && (ci->ndm_confirmed==0))
		    || ((ndm->ndm_state & NUD_STALE) && (ci->ndm_updated==0)
			 && (ci->ndm_used==0) && (ci->ndm_confirmed==15000))));
}

static inline detect_updated_neigh(struct ndmsg *ndm, struct nda_cacheinfo *ci,
				   int type)
{
	return ((type == RTM_NEWNEIGH) && (ndm->ndm_state & NUD_REACHABLE)
			&& (ci->ndm_updated == 0) && (ci->ndm_used != 0)
				&& (ci->ndm_confirmed != 0)
				&& (ci->ndm_confirmed == ci->ndm_used));
}

static inline detect_expired_neigh(struct ndmsg *ndm, struct nda_cacheinfo *ci,
				   int type)
{
	return (ndm->ndm_state & NUD_STALE) && (type == RTM_NEWNEIGH);
}

static inline detect_removed_neigh(struct ndmsg *ndm, struct nda_cacheinfo *ci,
				   int type)
{
	return (ndm->ndm_state & NUD_STALE) && (type == RTM_DELNEIGH);
}

static void handle_neigh_attrs(struct ndmsg *ndm, struct rtattr *tb[], int type)
{
	char ifname[IFNAMSIZ];
	void *addr = NULL, *lladdr;
	struct nda_cacheinfo * ci;

	if_indextoname(ndm->ndm_ifindex, ifname);

	if (tb[NDA_DST]) {
		addr = RTA_DATA(tb[NDA_DST]);
	}

	if (tb[NDA_LLADDR]) {
		lladdr = RTA_DATA(tb[NDA_LLADDR]);
	}

	if (tb[NDA_CACHEINFO]) {
		ci = RTA_DATA(tb[NDA_CACHEINFO]);
	}
	if (detect_new_neigh(ndm, ci, type)) {
		print_neigh_attrs(ndm, addr, lladdr, "Added", GREEN);
//		parse_ndm_state(ndm->ndm_state, ci);
	} else if (detect_updated_neigh(ndm, ci, type)) {
		print_neigh_attrs(ndm, addr, lladdr, "Updated", YELLOW);
	} else if (detect_expired_neigh(ndm, ci, type)) {
		print_neigh_attrs(ndm, addr, lladdr, "Expired", RED);
//		parse_ndm_state(ndm->ndm_state, ci);
	} else if (detect_removed_neigh(ndm, ci, type)) {
		print_neigh_attrs(ndm, addr, lladdr, "Removed", RED);
	} else {
		print_neigh_attrs(ndm, addr, lladdr, "Unknown action for", RED);
		parse_ndm_state(ndm->ndm_state, ci);
	}

}

static int handle_neigh_msg(struct nlmsghdr *nlh, int n)
{
	struct rtattr *tb[NDA_MAX];
	struct ndmsg *ndm = NLMSG_DATA(nlh);

	parse_rt_attrs(tb, NDA_MAX, RTM_RTA(ndm), RTM_PAYLOAD(nlh));
	handle_neigh_attrs(ndm, tb, nlh->nlmsg_type);

	return 0;
}

static void print_route_attrs(void *src, void *dst, void *gw, int *iif, int *oif,
		       struct rtmsg *rtm, char *action)
{
	char gw_str[INET6_ADDRSTRLEN], dst_str[INET6_ADDRSTRLEN];
	char src_str[INET6_ADDRSTRLEN], oif_str[IFNAMSIZ],
	    iif_str[IFNAMSIZ];
	int color, len;

	char buf[2048];

	color = (strcmp(action, "Added")?RED:GREEN);

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

	if (dst && src && oif && gw) {
		eprintf(color, "%s route %s/%d from %s/%d on dev %s via %s\n",
			action, dst_str, rtm->rtm_dst_len,
			src_str, rtm->rtm_src_len, oif_str, gw_str);
	} else if (dst && oif && gw) {
		eprintf(color, "%s route %s/%d on dev %s via %s\n",
			action, dst_str, rtm->rtm_dst_len,
			oif_str, gw_str);
	} else if (dst && oif) {
		eprintf(color, "%s route %s/%d on dev %s\n",
			action, dst_str, rtm->rtm_dst_len, oif_str);
	} else if (gw && oif) {
		eprintf(color, "%s default route via %s on dev %s\n",
			action, gw_str, oif_str);
	} else {
		len = sprintf(buf, "%s unknown route type:", action);
		if (gw)
			len += sprintf(buf+len, " gw");
		if (dst)
			len += sprintf(buf+len, " dst");
		if (src)
			len += sprintf(buf+len, " src");
		if (oif)
			len += sprintf(buf+len, " oif");
		if (iif)
			len += sprintf(buf+len, " iif");
		eprintf(color, "%s\n", buf);
	}
}

static int handle_route_attrs(struct rtmsg *rtm, struct rtattr *tb[], int type)
{
	void *src = NULL, *dst = NULL, *gw = NULL;
	int *iif = NULL, *oif = NULL;

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
	} else if (type == RTM_DELROUTE) {
		print_route_attrs(src, dst, gw, iif, oif, rtm, "Removed");
	}

	return 0;
}

static void parse_route_proto(struct rtmsg *rtm)
{
	switch (rtm->rtm_protocol) {
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
		printf("Route added by RA/RS mechanism\n");
		break;
	case RTPROT_ZEBRA:
		printf("route added by Zebra routing daemon\n");
		break;
	default:
		break;
	}
}

static int handle_route_msg(struct nlmsghdr *nlh, int n)
{
	struct rtmsg *rtm = NLMSG_DATA(nlh);
	struct rtattr *tb[RTN_MAX];

	parse_rt_attrs(tb, RTN_MAX, RTM_RTA(rtm), RTM_PAYLOAD(nlh));
	handle_route_attrs(rtm, tb, nlh->nlmsg_type);

	return 0;
}

static int handle_link_attrs(struct ifinfomsg * ifla, struct rtattr *tb[], int type)
{
	if (tb[IFLA_WIRELESS]) {
		tprintf("Found a Wireless Event\n");
	}

	return 0;
}

static int handle_link_msg(struct nlmsghdr *nlh, int n)
{
	struct ifinfomsg *ifla_msg = NLMSG_DATA(nlh);
	struct rtattr *tb[IFLA_MAX];
	char ifname[IFNAMSIZ];

	parse_rt_attrs(tb, IFLA_MAX, IFLA_RTA(ifla_msg), IFLA_PAYLOAD(nlh));
	if_indextoname(ifla_msg->ifi_index, ifname);
	parse_ifinfomsg(ifla_msg);

	handle_link_attrs(ifla_msg, tb, nlh->nlmsg_type);

	return 0;
}

int parse_rt_event( void *data, size_t n)
{
	struct nlmsghdr *nlh = (struct nlmsghdr *)data;

	switch (nlh->nlmsg_type) {
	case RTM_NEWLINK:
	case RTM_DELLINK:
	case RTM_GETLINK:
		handle_link_msg(nlh, n);
		break;
	case RTM_NEWADDR:
	case RTM_DELADDR:
	case RTM_GETADDR:
		handle_addr_msg(nlh, n);
		break;
	case RTM_NEWNEIGH:
	case RTM_DELNEIGH:
	case RTM_GETNEIGH:
		handle_neigh_msg(nlh, n);
		break;
	case RTM_NEWROUTE:
	case RTM_DELROUTE:
	case RTM_GETROUTE:
		handle_route_msg(nlh, n);
		break;
	default:
		eprintf(RED, "Unknown netlink event\n");
		break;
	}

	return 0;
}

static void init_opts(int * opts)
{
	memset(opts, 0, sizeof(int));
}

static void set_opt(int * opts, int opt)
{
	*opts |= opt;
}


static void parse_filters(char **argv, int start, int stop, int * filter)
{
	int f = 0;
	int pos = start;

	for (pos=start; pos<stop; pos++) {
		if (strcmp(argv[pos], "RTMGRP_LINK") == 0) {
			f |= RTMGRP_LINK;
		} else if (strcmp(argv[pos], "RTMGRP_NOTIFY") == 0) {
			f |= RTMGRP_NOTIFY;
		} else if (strcmp(argv[pos], "RTMGRP_NEIGH") == 0) {
			f |= RTMGRP_NEIGH;
		} else if (strcmp(argv[pos], "RTMGRP_IPV6_IFADDR") == 0) {
			f |= RTMGRP_IPV6_IFADDR;
		}  else if (strcmp(argv[pos], "RTMGRP_IPV6_ROUTE") == 0) {
			f |= RTMGRP_IPV6_ROUTE;
		}  else if (strcmp(argv[pos], "RTMGRP_IPV6_MROUTE") == 0) {
			f |= RTMGRP_IPV6_MROUTE;
		}  else if (strcmp(argv[pos], "RTMGRP_IPV6_IFINFO") == 0) {
			f |= RTMGRP_IPV6_IFINFO;
		}  else if (strcmp(argv[pos], "RTMGRP_IPV4_IFADDR") == 0) {
			f |= RTMGRP_IPV4_IFADDR;
		} else if (strcmp(argv[pos], "RTMGRP_IPV4_ROUTE") == 0) {
			f |= RTMGRP_IPV4_ROUTE;
		} else if (strcmp(argv[pos], "RTMGRP_IPV4_MROUTE") == 0) {
			f |= RTMGRP_IPV4_MROUTE;
		} else {
			printf("Invalid argument: %s\n", argv[pos]);
			exit(1);
		}
	}
	printf("Filter: ");
	for (pos=start; pos<stop; pos++) {
		printf("%s ", argv[pos]);
	} printf("\n");

	*filter = f;
}

static void parse_opts(int argc, char ** argv, int * opts, int * filter)
{
	int opt, idx=0;
	struct option lopts[] = {
		{"help", 0, 0, 'h'},
		{"color", 0, 0, 'c'},
	};

	init_opts(opts);

	if ( argc < 1 || argc > 5) {
		printf("Invalid arguments\n");
		short_header();
		exit(1);
	}

	while((opt = getopt_long(argc, argv, "hc", lopts, &idx)) != -1) {
		switch(opt) {
		case 0:
			break;
		case 'h':
			long_header();
			usage();
			exit(0);
			break;
		case 'c':
			set_opt(opts, OPT_COLOR);
			enable_color_output();
			break;
		default:
			exit(1);
			break;
		};
	}

	if (optind < argc) {
		parse_filters(argv, optind, argc, filter);
	}
}

int main(int argc, char ** argv)
{
	int sknl;
	struct event_handler ev_handler;

	int opts, filter;

	short_header();

	// default filter
	filter = DEFAULT_FILTER;

	parse_opts(argc, argv, &opts, &filter);

	if ( (sknl=setup_rtsocket(filter)) == -1 ) {
		printf("Error %d: %s\n", errno, strerror(errno));
		exit(1);
	}

	// Setup event handler
	event_init(&ev_handler);
	event_register(&ev_handler, parse_rt_event);

	// Install signal handlers
	signal(SIGHUP, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	// Register cleanup function
	atexit(exit_cleanup);

	if ( loop_rthandle(&ev_handler, sknl) != 0 ) {
		printf("Error %d: %s\n", errno, strerror(errno));
		exit(1);
	}

	close(sknl);
	return 0;
}
