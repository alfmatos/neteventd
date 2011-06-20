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

#include <stdio.h>
#include <netevent/utils.h>

int zero_addr(const unsigned char *addr)
{
        return (addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5]);
}

char * print_binary_stream(char * buf, unsigned int buflen, const unsigned char * data, unsigned int len)
{
	unsigned int i;
	unsigned int offset;

	for(i=0, offset=0; i<len, offset<buflen; i++) {
		offset += sprintf((buf+offset), "%02X", data[i]);
	}

	return buf;
}
