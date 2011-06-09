/*
 *  Copyright (C) 2008 IT Aveiro
 *  Copyright (C) 2011 Caixa Magica
 *
 *  Author:
 *
 *	Alfredo Matos <alfredo.matos@av.it.pt>
 *	Alfredo Matos <alfredo.matos@caixamagica.pt>
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
#include <netevent/console.h>
#include <netevent/rtnl.h>

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
		exit(1);
	}

	while((opt = getopt_long(argc, argv, "hc", lopts, &idx)) != -1) {
		switch(opt) {
		case 0:
			break;
		case 'h':
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

	fd_set rfds;

	//struct timeval tv;
	int retval, maxfd;


	FD_ZERO(&rfds);

	while(1) {

		maxfd = 0;
		FD_SET(sknl, &rfds);
		maxfd++;

		//tv.tv_sec = 1;
		//tv.tv_usec = 0;

		retval = select(maxfd, &rfds, NULL, NULL, NULL);

		if (retval == -1) {
			perror("select()\n");
			exit(1);
		}

		if (retval) {
			if(FD_ISSET(sknl, &rfds)) {
				if (recv_rtnl_msg(&ev_handler, sknl) != 0 ) {
					printf("Error %d: %s\n", errno, strerror(errno));
					exit(1);
				}
			}
		}
	}

	close(sknl);

	return 0;
}
