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

#include "nl80211-attrs.h"

struct nl_sock * gsock;

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

	nl_wait_for_ack(nlsk);

	nl_cb_put(cb);
	nlmsg_free(msg);

	return 0;
}

int nl_unparsed_ids(struct nlattr * tb[], unsigned int parsed[])
{
	int i, count, total, slen=0;

	char ids[256];

	total = nl_attr_count(tb);

	for(i=0, count=0; i<NL80211_ATTR_MAX; i++) {
		if(tb[i] && !parsed[i]) {
			if (count>0)
				slen +=	sprintf(ids+slen, ", ");
			slen += sprintf(ids+slen, "%d", i);
			count++;

			tprintf("mac80211: unparsed attribute %s(%d)\n",
				nl_attr_name[i], i);
		}
	}

	if (count < total) {
		tprintf("mac80211: parsed %d of %d attrs, unparsed_ids(%s)\n",
		count, total, ids);
	}

	return count;
}

int nl80211_handle_attrs(unsigned int cmd, struct nlattr * tb[])
{
	char ifname[IFNAMSIZ]="null";
	char addr_str[INET6_ADDRSTRLEN];
	unsigned int ifindex, status, wiphy;

	unsigned int parsed[NL80211_ATTR_MAX + 1];

	memset(parsed, 0, sizeof(int)*(NL80211_ATTR_MAX+1));

	if (tb[NL80211_ATTR_WIPHY]) {
		wiphy = nla_get_u32(tb[NL80211_ATTR_WIPHY]);
		parsed[NL80211_ATTR_WIPHY] = 1;
	}

	if (tb[NL80211_ATTR_IFINDEX]) {
		ifindex = nla_get_u32(tb[NL80211_ATTR_IFINDEX]);
		if_indextoname(ifindex, ifname);
		parsed[NL80211_ATTR_IFINDEX] = 1;
	}

	switch(cmd) {
	case NL80211_CMD_GET_WIPHY:
	case NL80211_CMD_SET_WIPHY:
	case NL80211_CMD_NEW_WIPHY:
	case NL80211_CMD_DEL_WIPHY:
	case NL80211_CMD_SET_WIPHY_NETNS:
		tprintf("mac80211 wiphy mgmt\n");
		break;
	case NL80211_CMD_GET_INTERFACE:
	case NL80211_CMD_SET_INTERFACE:
	case NL80211_CMD_NEW_INTERFACE:
	case NL80211_CMD_DEL_INTERFACE:
		tprintf("mac80211 interface mgmt\n");
		break;
	case NL80211_CMD_GET_KEY:
	case NL80211_CMD_SET_KEY:
	case NL80211_CMD_NEW_KEY:
	case NL80211_CMD_DEL_KEY:
		tprintf("mac80211 key mgmt\n");
		break;

	case NL80211_CMD_GET_BEACON:
	case NL80211_CMD_SET_BEACON:
        case NL80211_CMD_NEW_BEACON:
	case NL80211_CMD_DEL_BEACON:
	case NL80211_CMD_REG_BEACON_HINT:
		tprintf("mac80211: beacon mgmt\n");
		break;
	case NL80211_CMD_GET_STATION:
		ether_ntoa_r(nla_data(tb[NL80211_ATTR_MAC]), addr_str);
		tprintf("mac80211: station %s on %s get attributes\n",
			addr_str, ifname);
		parsed[NL80211_ATTR_MAC] = 1;
		break;
	case NL80211_CMD_SET_STATION:
		ether_ntoa_r(nla_data(tb[NL80211_ATTR_MAC]), addr_str);
		tprintf("mac80211: station %s on %s set attributes\n",
			addr_str, ifname);
		parsed[NL80211_ATTR_MAC] = 1;
		break;
	case NL80211_CMD_NEW_STATION:
		ether_ntoa_r(nla_data(tb[NL80211_ATTR_MAC]), addr_str);
		tprintf("mac80211: add station %s on %s\n",
			addr_str, ifname);
		parsed[NL80211_ATTR_MAC] = 1;
		break;
	case NL80211_CMD_DEL_STATION:
		ether_ntoa_r(nla_data(tb[NL80211_ATTR_MAC]), addr_str);
		tprintf("mac80211: remove station %s on %s\n",
			addr_str, ifname);
		parsed[NL80211_ATTR_MAC] = 1;
		break;
	case NL80211_CMD_GET_MPATH:
	case NL80211_CMD_SET_MPATH:
	case NL80211_CMD_NEW_MPATH:
	case NL80211_CMD_DEL_MPATH:
		tprintf("mac80211: mpath mgmt\n");
		break;
	case NL80211_CMD_SET_BSS:
	case NL80211_CMD_SET_MGMT_EXTRA_IE:
	case NL80211_CMD_JOIN_IBSS:
	case NL80211_CMD_LEAVE_IBSS:
		tprintf("mac80211: bss mgmt\n");
		break;
	case NL80211_CMD_GET_MESH_CONFIG:
	case NL80211_CMD_SET_MESH_CONFIG:
	case NL80211_CMD_JOIN_MESH:
	case NL80211_CMD_LEAVE_MESH:
		tprintf("mac80211: mesh mgmt\n");
		break;
	case NL80211_CMD_GET_SCAN:
		tprintf("mac80211: scan get\n");
		break;
	case NL80211_CMD_TRIGGER_SCAN:
		tprintf("mac80211: scan triggered on %s\n", ifname);
		break;
	case NL80211_CMD_NEW_SCAN_RESULTS:
		tprintf("mac80211: scan finished on %s\n", ifname);
		break;
	case NL80211_CMD_SCAN_ABORTED:
		tprintf("mac80211: scan abort\n");
		break;
	case NL80211_CMD_GET_REG:
		tprintf("mac80211: get reg on %s\n", ifname);
		break;
	case NL80211_CMD_SET_REG:
		tprintf("mac80211: set reg on %s\n", ifname);
		break;
	case NL80211_CMD_REG_CHANGE:
		tprintf("mac80211: change reg on %s\n", ifname);
		break;
	case NL80211_CMD_REQ_SET_REG:
		tprintf("mac80211: request set reg on %s\n", ifname);
		break;
	case NL80211_CMD_AUTHENTICATE:
		tprintf("mac80211: authenticate\n");
		break;
	case NL80211_CMD_ASSOCIATE:
		if (tb[NL80211_ATTR_MAC]) {
			ether_ntoa_r(nla_data(tb[NL80211_ATTR_MAC]), addr_str);
			tprintf("mac80211: associate to %s on %s\n",
				addr_str,
				ifname);
			parsed[NL80211_ATTR_MAC] = 1;
		} else {
			tprintf("mac80211: associated on %s\n", ifname);
		}
		break;
	case NL80211_CMD_DEAUTHENTICATE:
		tprintf("mac80211: deauthenticate\n");
		break;
	case NL80211_CMD_DISASSOCIATE:
		tprintf("mac80211: disassociate\n");
		break;
	case NL80211_CMD_MICHAEL_MIC_FAILURE:
		tprintf("mac80211: mic fail mgmt\n");
		break;
	case NL80211_CMD_TESTMODE:
		tprintf("mac80211 connection testmode\n");
		break;
	case NL80211_CMD_CONNECT:
		status = 0;

		if (tb[NL80211_ATTR_STATUS_CODE]) {
			status = nla_get_u16(tb[NL80211_ATTR_STATUS_CODE]);
			parsed[NL80211_ATTR_STATUS_CODE] = 1;
		}

		if(tb[NL80211_ATTR_SSID])
			tprintf("mac80211: attribute SSID\n");

		if(tb[NL80211_ATTR_MAC]) {
			ether_ntoa_r(nla_data(tb[NL80211_ATTR_MAC]), addr_str);
			tprintf("mac80211: connected to %s on %s\n",
				addr_str, ifname);
			parsed[NL80211_ATTR_MAC] = 1;
		} else {
			tprintf("Failed to connect - status %d\n", status);
		}

		break;
	case NL80211_CMD_ROAM:
		tprintf("mac80211: connection roamed on %s\n", ifname);
		break;
	case NL80211_CMD_DISCONNECT:
		status = 0;

		if (tb[NL80211_ATTR_REASON_CODE]) {
			status = nla_get_u16(tb[NL80211_ATTR_REASON_CODE]);
			tprintf("mac80211: disconnected on %s (%d)\n",
				ifname, status);
			parsed[NL80211_ATTR_REASON_CODE] = 1;
		} else {
			tprintf("mac80211: disconnected on %s\n", ifname);
		}
		break;
	case NL80211_CMD_GET_SURVEY:
	case NL80211_CMD_NEW_SURVEY_RESULTS:
		tprintf("mac80211 survey mgmt\n");
		break;
	case NL80211_CMD_SET_PMKSA:
	case NL80211_CMD_DEL_PMKSA:
	case NL80211_CMD_FLUSH_PMKSA:
		tprintf("mac80211 pmksa mgmt\n");
		break;
	case NL80211_CMD_REMAIN_ON_CHANNEL:
        case NL80211_CMD_CANCEL_REMAIN_ON_CHANNEL:
	case NL80211_CMD_SET_CHANNEL:
		tprintf("mac80211 channel mgmt\n");
		break;
        case NL80211_CMD_SET_TX_BITRATE_MASK:
		tprintf("mac80211 bit rate mask\n");
		break;
        case NL80211_CMD_REGISTER_FRAME:
	case NL80211_CMD_FRAME:
	case NL80211_CMD_FRAME_TX_STATUS:
		tprintf("mac80211 frame mgmt\n");
		break;
	case NL80211_CMD_SET_POWER_SAVE:
	case NL80211_CMD_GET_POWER_SAVE:
		tprintf("mac80211 power mgmt\n");
		break;
	case NL80211_CMD_SET_CQM:
	case NL80211_CMD_NOTIFY_CQM:
		tprintf("mac80211 cqm mgmt\n");
		break;
	case NL80211_CMD_SET_WDS_PEER:
	case NL80211_CMD_FRAME_WAIT_CANCEL:
		break;
	case NL80211_CMD_UNPROT_DEAUTHENTICATE:
	case NL80211_CMD_UNPROT_DISASSOCIATE:
		tprintf("mac80211 unprot fail\n");
		break;

	default:
		tprintf("mac80211: Unknown event\n");
		break;
	}

	return nl_unparsed_ids(tb, parsed);
}

int nl_attr_count(struct nlattr * tb[])
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

	n = nl_attr_count(tb);

	count = nl80211_handle_attrs(genlh->cmd, tb);

	return NL_OK;
}

int nl80211_socket_init(void)
{
        int i, id;
	struct nl_cb * cb;

        gsock = nl_socket_alloc();

        genl_connect(gsock);
        id = genl_ctrl_resolve(gsock, "nl80211");

        nl80211_register_multicast_groups(gsock, id);

        nl_socket_disable_seq_check(gsock);
	nl_socket_modify_cb(gsock, NL_CB_VALID, NL_CB_CUSTOM, nl80211_handle_event, NULL);

        return nl_socket_get_fd(gsock);
}

int nl80211_socket_close(struct nl_sock * nlsk)
{
	nl_socket_free(nlsk);
	return 0;
}

inline int nl80211_msg_rx(int skfd)
{
	nl_recvmsgs_default(gsock);
}
