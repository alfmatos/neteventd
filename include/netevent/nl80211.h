#ifndef __NETEVENT_NL80211__
#define __NETEVENT_NL80211__

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>

int nl80211_socket_init();
int nl80211_socket_close(struct nl_sock * nlsk);
int nl80211_msg_rx(int nlsk);

#endif
