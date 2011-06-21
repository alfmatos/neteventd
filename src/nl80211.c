/*
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
 *
 *  Copyright (C) 2011 Caixa Magica
 *
 *  Author: Alfredo Matos <alfredo.matos@caixamagica.pt>
 */

#include <netevent/nl80211.h>
#include <netevent/console.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/family.h>
#include <netlink/netlink.h>

#include <linux/nl80211.h>

struct nl_sock * gsock;
struct nl_cb * gcb;

int join_multicast_group(struct nl_sock * nlsk, int id)
{
	if (id<0)
		return id;

	return nl_socket_add_membership(nlsk, id);
}

int nl80211_register_mc_attr(struct nl_sock * nlsk, struct nlattr * mc_attr)
{
	struct nlattr *tb[CTRL_ATTR_MCAST_GRP_MAX + 1];
	struct nlattr * data;
	int len, mcid;

	data = nla_data(mc_attr);
	len = nla_len(mc_attr);

	nla_parse(tb, CTRL_ATTR_MCAST_GRP_MAX, data, len, NULL);

	if(tb[CTRL_ATTR_MCAST_GRP_ID]) {
		mcid = nla_get_u32(tb[CTRL_ATTR_MCAST_GRP_ID]);
		join_multicast_group(nlsk, mcid);

#ifdef DEBUG
		if(tb[CTRL_ATTR_MCAST_GRP_NAME]) {
			tprintf("Registered nl80211 mc group: %s\n",
				nla_data(tb[CTRL_ATTR_MCAST_GRP_NAME]));
		}
#endif
	}

	return 0;
}

int nl80211_register_mc_groups(struct nl_msg * msg, void *arg)
{
	struct nl_sock * nlsk = arg;
	int status;
	struct nlattr *tb[CTRL_ATTR_MAX + 1];
	struct genlmsghdr * genlh;

	struct nlattr * mc_attr, * attrdata;
	int rem, attrlen;

	genlh = nlmsg_data(nlmsg_hdr(msg));

	attrdata = genlmsg_attrdata(genlh, 0);
	attrlen = genlmsg_attrlen(genlh, 0);

	nla_parse(tb, CTRL_ATTR_MAX, attrdata, attrlen, NULL);

	if(tb[CTRL_ATTR_MCAST_GROUPS]) {
		nla_for_each_nested(mc_attr, tb[CTRL_ATTR_MCAST_GROUPS], rem) {
			status = nl80211_register_mc_attr(nlsk, mc_attr);
		}
	}

	return NL_SKIP;
}


int nl80211_register_multicast_groups(struct nl_sock * nlsk, int fid)
{
	struct nl_msg * msg;
	struct nl_cb * cb;
	int cid;

	cid = genl_ctrl_resolve(nlsk, "nlctrl");

	msg = nlmsg_alloc();

	genlmsg_put(msg, 0, 0, cid, 0, 0, CTRL_CMD_GETFAMILY, 0);
	nla_put_u16(msg, CTRL_ATTR_FAMILY_ID, fid);

	nl_send_auto_complete(nlsk, msg);

	cb = nl_cb_alloc(NL_CB_VERBOSE);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM,
			nl80211_register_mc_groups, nlsk);

	nl_recvmsgs(nlsk, cb);

	nl_cb_put(cb);
	nlmsg_free(msg);

	return 0;
}

int nl80211_handle_attrs(struct nlattr * tb)
{
	int count;

	count = 0;

	return count;
}

int get_nl_attr_count(struct nlattr * tb[])
{
	int i, count;

	for(i=0, count=0; i<NL80211_ATTR_MAX; i++) {
		if(tb[i]) {
			count++;
		}
	}

	return count;
}

int nl80211_handle_event(struct nl_msg * msg, void * arg)
{
	struct genlmsghdr * genlh;
	struct nlattr * tb[NL80211_ATTR_MAX + 1];
	struct nlattr * attrdata, * nla;
	int attrlen, n, count;

	genlh = nlmsg_data(nlmsg_hdr(msg));

	attrdata = genlmsg_attrdata(genlh, 0);
	attrlen = genlmsg_attrlen(genlh, 0);

	nla_parse(tb, NL80211_ATTR_MAX, attrdata, attrlen, NULL);

	n = get_nl_attr_count(tb);

	count = nl80211_handle_attrs(tb);

	if(count<n) {
		tprintf("mac80211: Processed %d of %d nl80211 attributes\n",
			count, n);
	}

	return NL_OK;
}

static int seq_check_pass(struct nl_msg *msg, void *arg)
{
	return NL_OK;
}

int nl80211_register_callbacks(struct nl_cb ** cb)
{
	*cb = nl_cb_alloc(NL_CB_VERBOSE);

	nl_cb_set(*cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, seq_check_pass, NULL);
	nl_cb_set(*cb, NL_CB_VALID, NL_CB_CUSTOM, nl80211_handle_event, NULL);

	return 0;
}

int nl80211_socket_init(void)
{
	int i, id;

	gsock = nl_socket_alloc();

	genl_connect(gsock);

	id = genl_ctrl_resolve(gsock, "nl80211");

	nl80211_register_multicast_groups(gsock, id);
	nl80211_register_callbacks(&gcb);

	return nl_socket_get_fd(gsock);
}

int nl80211_socket_close(struct nl_sock * nlsk)
{
	nl_socket_free(nlsk);
	return 0;
}

inline int nl80211_msg_rx(int skfd)
{
	nl_recvmsgs(gsock, gcb);
}
