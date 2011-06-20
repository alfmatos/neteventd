/*
 *  Copyright (C) 2011 Caixa Magica
 *
 *  Author: Alfredo Matos <alfredo.matos@caixamagica.pt>
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

#include <netevent/iw.h>
#include <netevent/console.h>
#include <netevent/utils.h>

#include <netinet/ether.h>

struct iw_range * get_iw_range(int ifindex)
{
	char ifname[IFNAMSIZ];
	struct iw_range * range;
	int skfd;

	if_indextoname(ifindex, ifname);

	skfd = iw_sockets_open();

	range = malloc(sizeof(struct iw_range));

	iw_get_range_info(skfd, ifname, range);

	iw_sockets_close(skfd);

	return range;
}

int handle_wireless_event(int ifindex, struct iw_event * iwe)
{
	char ifname[IFNAMSIZ];
	unsigned int ev_len;
	int idx = 0;
	char essid[4*IW_ESSID_MAX_SIZE + 1];
	char buffer[128];

	struct ether_addr * ap_addr;
	struct sockaddr * sa_ap;
	struct iw_range * range;
	struct iw_point * id;

	double freq;
	int channel;

	char ap_str[18];
	char freq_str[16];

	char * udata, * pdata;
	unsigned int ulen;

	if_indextoname(ifindex, ifname);

	switch (iwe->cmd)
	{
	case SIOCGIWSCAN:
		tprintf("Wireless scan completed on %s\n", ifname);
		break;
	case SIOCSIWESSID:
	case SIOCGIWESSID:
		id = (struct iw_point *) &iwe->u.essid;

		if((id->pointer) && (id->length) && id->flags) {
			iw_essid_escape(essid, iwe->u.essid.pointer, iwe->u.essid.length);
			tprintf("ESSID set to \"%s\" on %s\n", essid, ifname);
		} else {
			tprintf("ESSID on %s set to off/any\n", ifname);
		}
		break;
	case SIOCSIWMODE:
		tprintf("Updated %s to mode %s\n", ifname, iw_operation_mode[iwe->u.mode]);
		break;
	case SIOCSIWFREQ:
		range = get_iw_range(ifindex);
		freq = iw_freq2float(&(iwe->u.freq));

		if (freq < 10e3) {
			channel = iw_channel_to_freq((int) freq, &freq, range);
		} else {
			channel = iw_freq_to_channel(freq, range);
		}

		iw_print_freq_value(freq_str, sizeof(freq_str), freq);

		tprintf("Updated %s to frequency %s and channel %d\n", ifname, freq_str, channel);
		break;
	case SIOCGIWAP:
		sa_ap = (struct sockaddr *) &iwe->u.ap_addr;
		ap_addr = (struct ether_addr *) sa_ap->sa_data;

		if (!zero_addr((unsigned char *) ap_addr)) {
			tprintf("%s Lost Association\n", ifname);
		} else {
			ether_ntoa_r(ap_addr, ap_str);
			tprintf("New AP Address [%s] on %s\n", ap_str ,ifname);
		}
		break;
	case IWEVASSOCRESPIE:
		udata = iwe->u.data.pointer;
		ulen = iwe->u.data.length;

		pdata = print_binary_stream(buffer, 8, udata, ulen);
		tprintf("%s received an Association Response (0x%s...)\n", ifname, pdata);
		break;
	default:
		tprintf("Wireless event 0x%X, len %d\n", iwe->cmd, iwe->len);
	}

	return 0;
}

int handle_wireless_attr(int ifindex, char * data, int len)
{
	struct iw_event iwe;
	char ifname[IFNAMSIZ];
	struct stream_descr stream;

	if_indextoname(ifindex, ifname);
	iw_init_event_stream(&stream, data, len);

	while(iw_extract_event_stream(&stream, &iwe, WIRELESS_EXT) > 0) {
		handle_wireless_event(ifindex, &iwe);
	}

	return 0;
}
