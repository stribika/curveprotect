/*  Copyright (C) 2011 CZ.NIC, z.s.p.o. <knot-dns@labs.nic.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>

#include "binary.h"
#include "common/base64.h"
#include "common/errcode.h"

int knot_binary_from_base64(const char *base64, knot_binary_t *binary)
{
	if (!binary)
		return KNOT_EINVAL;

	uint8_t *data;
	int32_t size;

	size = base64_decode_alloc((uint8_t *)base64, strlen(base64), &data);
	if (size < 0) {
		return size;
	}

	binary->data = data;
	binary->size = size;

	return KNOT_EOK;
}

int knot_binary_free(knot_binary_t *binary)
{
	if (!binary)
		return KNOT_EINVAL;

	if (binary->data) {
		free(binary->data);
		binary->data = NULL;
		binary->size = 0;
	}

	return KNOT_EOK;
}
