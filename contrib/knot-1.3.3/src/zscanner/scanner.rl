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

#include "zscanner/scanner.h"

#include <stdint.h>			// uint32_t
#include <stdlib.h>			// calloc
#include <stdio.h>			// sprintf
#include <libgen.h>			// dirname
#include <stdbool.h>			// bool
#include <math.h>			// pow
#include <string.h>			// strdup
#include <sys/types.h>			// (OpenBSD)
#include <sys/socket.h>			// AF_INET (BSD)
#include <netinet/in.h>			// in_addr (BSD)
#include <arpa/inet.h>			// inet_pton

#include "common/errcode.h"		// error codes
#include "common/descriptor.h"		// KNOT_RRTYPE_A
#include "zscanner/file_loader.h"	// file_loader
#include "zscanner/scanner_functions.h"	// Base64

/*! \brief Shorthand for setting warning data. */
#define SCANNER_WARNING(code) { s->error_code = code; }
/*! \brief Shorthand for setting error data. */
#define SCANNER_ERROR(code)   { s->error_code = code; s->stop = true; }

/*!
 * \brief Writes record type number to r_data.
 *
 * \param type		Type number.
 * \param rdata_tail	Position where to write type number to.
 */
static inline void type_num(const uint16_t type, uint8_t **rdata_tail)
{
	*((uint16_t *)*rdata_tail) = htons(type);
	*rdata_tail += 2;
}

/*!
 * \brief Sets bit to bitmap window.
 *
 * \param type		Type number.
 * \param s		Scanner context.
 */
static inline void window_add_bit(const uint16_t type, scanner_t *s) {
	uint8_t win      = type / 256;
	uint8_t bit_pos  = type % 256;
	uint8_t byte_pos = bit_pos / 8;

	((s->windows[win]).bitmap)[byte_pos] |= 128 >> (bit_pos % 8);

	if ((s->windows[win]).length < byte_pos + 1) {
		(s->windows[win]).length = byte_pos + 1;
	}

	if (s->last_window < win) {
		s->last_window = win;
	}
}

// Include scanner file (in Ragel).
%%{
	machine zone_scanner;

	include "scanner_body.rl";

	write data;
}%%

scanner_t* scanner_create(const char *file_name)
{
	scanner_t *s = calloc(1, sizeof(scanner_t));
	if (s == NULL) {
		return NULL;
	}

	s->file_name = strdup(file_name);
	s->line_counter = 1;

	// Nonzero initial scanner state.
	s->cs = zone_scanner_start;

	return s;
}

void scanner_free(scanner_t *s)
{
	if (s != NULL) {
		free(s->file_name);
		free(s);
	}
}

int scanner_process(const char *start,
		    const char *end,
		    const bool is_last_block,
		    scanner_t  *s)
{
	// Necessary scanner variables.
	const char *p = start;
	const char *pe = end;
	char       *eof = NULL;
	int        stack[RAGEL_STACK_SIZE];

	// Auxiliary variables which are used in scanner body.
	struct in_addr  addr4;
	struct in6_addr addr6;
	uint32_t timestamp;
	int16_t  window;
	int	 ret;

	// Next 2 variables are for better performance.
	// Restoring r_data pointer to next free space.
	uint8_t *rdata_tail = s->r_data + s->r_data_tail;
	// Initialization of the last r_data byte.
	uint8_t *rdata_stop = s->r_data + MAX_RDATA_LENGTH - 1;

	// Restoring scanner states.
	int cs  = s->cs;
	int top = s->top;
	memcpy(stack, s->stack, sizeof(stack));

	// End of file check.
	if (is_last_block == true) {
		eof = (char *)pe;
	}

	// Writing scanner body (in C).
	%% write exec;

	// Check if scanner state machine is in uncovered state.
	if (cs == zone_scanner_error) {
		SCANNER_ERROR(ZSCANNER_UNCOVERED_STATE);
		s->error_counter++;

		// Fill error context data.
		for (s->buffer_length = 0;
		     ((p + s->buffer_length) < pe) &&
		     (s->buffer_length < sizeof(s->buffer) - 1);
		     s->buffer_length++)
		{
			// Only rest of the current line.
			if (*(p + s->buffer_length) == '\n') {
				break;
			}
			s->buffer[s->buffer_length] = *(p + s->buffer_length);
		}

		// Ending string in buffer.
		s->buffer[s->buffer_length++] = 0;

		// Processing error.
		s->process_error(s);

		return -1;
	}

	// Check unclosed multiline record.
	if (is_last_block && s->multiline) {
		SCANNER_ERROR(ZSCANNER_UNCLOSED_MULTILINE);
		s->error_counter++;
		s->process_error(s);
	}

	// Storing scanner states.
	s->cs  = cs;
	s->top = top;
	memcpy(s->stack, stack, sizeof(stack));

	// Storing r_data pointer.
	s->r_data_tail = rdata_tail - s->r_data;

	// Check if any errors has occured.
	if (s->error_counter > 0) {
		return -1;
	}

	return 0;
}
