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

/*!
 * \file includes.h
 *
 * \author Jan Vcelak <jan.vcelak@nic.cz>
 *
 * \brief Structures for binary data handling.
 *
 * \addtogroup libknot
 * @{
 */

#ifndef _KNOT_BINARY_H_
#define _KNOT_BINARY_H_

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

struct knot_binary {
	uint8_t *data;
	size_t size;
};

typedef struct knot_binary knot_binary_t;

/*!
 * \brief Initialize knot_binary_t structure from Base64 encoded string.
 *
 * \param base64  Base64 encoded input data.
 * \param binary  Pointer to structure to write the result into.
 * \return Error code, KNOT_EOK in case of success.
 */
int knot_binary_from_base64(const char *base64, knot_binary_t *binary);

/*!
 * \brief Free content of knot_binary_t structure.
 *
 * \param binary  Pointer to the structure.
 * \return Error code, KNOT_EOK in case of success.
 */
int knot_binary_free(knot_binary_t *binary);

#endif // _KNOT_BINARY_H

/*! @} */
