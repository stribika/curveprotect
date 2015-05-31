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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <time.h>

#include "tsig.h"
#include "util/debug.h"
#include "common.h"
#include "util/utils.h"
#include "rrset.h"
#include "dname.h"
#include "consts.h"

/*! \brief TSIG field offsets. */
typedef enum tsig_off_t {
	TSIG_ALGNAME_O = 0,
	TSIG_TSIGNED_O,
	TSIG_FUDGE_O,
	TSIG_MACLEN_O,
	TSIG_MAC_O,
	TSIG_ORIGID_O,
	TSIG_ERROR_O,
	TSIG_OLEN_O,
	TSIG_OTHER_O
} tsig_off_t;

/* Helpers for r offset calculation. */
#define TSIG_NAMELEN (sizeof(knot_dname_t*))
#define TSIG_OTHER_MAXLEN (3 * sizeof(uint16_t))
#define TSIG_OFF_MACLEN (TSIG_NAMELEN + 4 * sizeof(uint16_t))
#define TSIG_FIXED_RDLEN (TSIG_NAMELEN + 11 * sizeof(uint16_t))

/*!
 * \brief Seek offset of a TSIG RR field.
 *
 * \param rr TSIG RR.
 * \param id Field index.
 * \param nb Required number of bytes after the offset (for boundaries check).
 * \return pointer to field on wire or NULL.
 */
static uint8_t* tsig_rdata_seek(const knot_rrset_t *rr, tsig_off_t id, size_t nb)
{
	uint8_t *rd = knot_rrset_get_rdata(rr, 0);
	if (rd == NULL) {
		return NULL;
	}

	/* Check if fixed part is readable. */
	uint16_t lim = rrset_rdata_item_size(rr, 0);
	if (lim < TSIG_NAMELEN + 5 * sizeof(uint16_t)) {
		dbg_tsig("TSIG: rdata: not enough items "
		         "(has %"PRIu16", min %zu).\n",
		         lim, TSIG_NAMELEN + 5 * sizeof(uint16_t));
		return NULL;
	}

	/* Not pretty, but fast. */
	uint8_t *bp = rd;
	switch(id) {
	case TSIG_ALGNAME_O: break;
	case TSIG_TSIGNED_O: rd += TSIG_NAMELEN; break;
	case TSIG_FUDGE_O: rd += TSIG_NAMELEN + 3 * sizeof(uint16_t); break;
	case TSIG_MACLEN_O: rd += TSIG_NAMELEN + 4 * sizeof(uint16_t); break;
	case TSIG_MAC_O: rd += TSIG_NAMELEN + 5 * sizeof(uint16_t); break;
	case TSIG_ORIGID_O:
		rd += TSIG_NAMELEN + 4 * sizeof(uint16_t);
		rd += knot_wire_read_u16(rd) + sizeof(uint16_t);
		break;

	case TSIG_ERROR_O:
		rd += TSIG_NAMELEN + 4 * sizeof(uint16_t);
		rd += knot_wire_read_u16(rd) + 2 * sizeof(uint16_t);
		break;
	case TSIG_OLEN_O:
		rd += TSIG_NAMELEN + 4 * sizeof(uint16_t);
		rd += knot_wire_read_u16(rd) + 3 * sizeof(uint16_t);
		break;
	case TSIG_OTHER_O:
		rd += TSIG_NAMELEN + 4 * sizeof(uint16_t);
		rd += knot_wire_read_u16(rd) + 4 * sizeof(uint16_t);
		break;
	}

	/* Check remaining bytes. */
	if (rd + nb > bp + lim) {
		dbg_tsig("TSIG: rdata: not enough items (needs %zu, has %u).\n",
		         (rd-bp)+nb, lim);
		return NULL;
	}

	return rd;
}

static int tsig_rdata_set_tsig_error(knot_rrset_t *tsig, uint16_t tsig_error)
{
	uint8_t *rd = tsig_rdata_seek(tsig, TSIG_ERROR_O, sizeof(uint16_t));
	if (!rd) {
		return KNOT_ERROR;
	}

	knot_wire_write_u16(rd, tsig_error);
	return KNOT_EOK;
}

int tsig_create_rdata(knot_rrset_t *rr, uint16_t maclen, uint16_t tsig_err)
{
	if (!rr) {
		return KNOT_EINVAL;
	}

	/* We already checked rr and know rdlen > 0, no need to check rets. */
	size_t rdlen = TSIG_FIXED_RDLEN + maclen;
	if (tsig_err != KNOT_RCODE_BADTIME) {
		rdlen -= TSIG_OTHER_MAXLEN;
	}
	uint8_t *rd = knot_rrset_create_rdata(rr, rdlen);
	memset(rd, 0, rdlen);

	/* Set MAC variable length in advance. */
	rd += TSIG_OFF_MACLEN;
	knot_wire_write_u16(rd, maclen);

	/* Set error. */
	tsig_rdata_set_tsig_error(rr, tsig_err);

	return KNOT_EOK;
}

int tsig_rdata_set_alg_name(knot_rrset_t *tsig, knot_dname_t *alg_name)
{
	uint8_t *rd = tsig_rdata_seek(tsig, TSIG_ALGNAME_O, TSIG_NAMELEN);
	if (!rd) {
		return KNOT_ERROR;
	}

	memcpy(rd, &alg_name, sizeof(knot_dname_t*));
	knot_dname_retain(alg_name);
	return KNOT_EOK;
}

int tsig_rdata_set_alg(knot_rrset_t *tsig, knot_tsig_algorithm_t alg)
{
	const char *s = tsig_alg_to_str(alg);
	knot_dname_t *alg_name = knot_dname_new_from_str(s, strlen(s), NULL);
	int ret = tsig_rdata_set_alg_name(tsig, alg_name);
	knot_dname_release(alg_name);
	return ret;
}

int tsig_rdata_set_time_signed(knot_rrset_t *tsig, uint64_t time)
{
	uint8_t *rd = tsig_rdata_seek(tsig, TSIG_TSIGNED_O, 3*sizeof(uint16_t));
	if (!rd) {
		return KNOT_ERROR;
	}

	knot_wire_write_u48(rd, time);
	return KNOT_EOK;
}

int tsig_rdata_set_fudge(knot_rrset_t *tsig, uint16_t fudge)
{
	uint8_t *rd = tsig_rdata_seek(tsig, TSIG_FUDGE_O, sizeof(uint16_t));
	if (!rd) {
		return KNOT_ERROR;
	}

	knot_wire_write_u16(rd, fudge);
	return KNOT_EOK;
}

int tsig_rdata_set_mac(knot_rrset_t *tsig, uint16_t length, const uint8_t *mac)
{
	uint8_t *rd = tsig_rdata_seek(tsig, TSIG_MAC_O, length);
	if (!rd) {
		return KNOT_ERROR;
	}

	/*! \note Cannot change length, as rdata is already preallocd. */

	/* Copy the actual MAC. */
	memcpy(rd, mac, length);
	return KNOT_EOK;
}

int tsig_rdata_set_orig_id(knot_rrset_t *tsig, uint16_t id)
{
	uint8_t *rd = tsig_rdata_seek(tsig, TSIG_ORIGID_O, sizeof(uint16_t));
	if (!rd) {
		return KNOT_ERROR;
	}

	/* Write the length - 2. */
	knot_wire_write_u16(rd, id);
	return KNOT_EOK;
}

int tsig_rdata_set_other_data(knot_rrset_t *tsig, uint16_t len,
                              const uint8_t *other_data)
{
	if (len > TSIG_OTHER_MAXLEN) {
		dbg_tsig("TSIG: rdata: other len > %zu B\n", TSIG_OTHER_MAXLEN);
		return KNOT_EINVAL;
	}

	uint8_t *rd = tsig_rdata_seek(tsig, TSIG_OLEN_O, len+sizeof(uint16_t));
	if (!rd) {
		return KNOT_ERROR;
	}

	/* Write the length. */
	knot_wire_write_u16(rd, len);

	/* Copy the actual data. */
	memcpy(rd + sizeof(uint16_t), other_data, len);
	return KNOT_EOK;
}

const knot_dname_t *tsig_rdata_alg_name(const knot_rrset_t *tsig)
{
	uint8_t *rd = tsig_rdata_seek(tsig, TSIG_ALGNAME_O, TSIG_NAMELEN);
	if (!rd) {
		return NULL;
	}
	return *((knot_dname_t**)rd);
}

knot_tsig_algorithm_t tsig_rdata_alg(const knot_rrset_t *tsig)
{
	/* Get the algorithm name. */
	const knot_dname_t *alg_name = tsig_rdata_alg_name(tsig);
	if (!alg_name) {
		dbg_tsig("TSIG: rdata: cannot get algorithm name.\n");
		return KNOT_TSIG_ALG_NULL;
	}

	/* Convert alg name to string. */
	char *name = knot_dname_to_str(alg_name);
	if (!name) {
		dbg_tsig("TSIG: rdata: cannot convert alg name.\n");
		return KNOT_TSIG_ALG_NULL;
	}

	knot_lookup_table_t *item = knot_lookup_by_name(
	                                      knot_tsig_alg_domain_names, name);
	free(name);
	if (!item) {
		dbg_tsig("TSIG: rdata: unknown algorithm.\n");
		return KNOT_TSIG_ALG_NULL;
	}
	return item->id;
}

uint64_t tsig_rdata_time_signed(const knot_rrset_t *tsig)
{
	/*! \todo How to return invalid value? */
	uint8_t *rd = tsig_rdata_seek(tsig, TSIG_TSIGNED_O, 3*sizeof(uint16_t));
	if (!rd) {
		return 0;
	}
	return knot_wire_read_u48(rd);
}

uint16_t tsig_rdata_fudge(const knot_rrset_t *tsig)
{
	uint8_t *rd = tsig_rdata_seek(tsig, TSIG_FUDGE_O, sizeof(uint16_t));
	if (!rd) {
		return 0;
	}
	return knot_wire_read_u16(rd);
}

const uint8_t *tsig_rdata_mac(const knot_rrset_t *tsig)
{
	uint8_t *rd = tsig_rdata_seek(tsig, TSIG_MAC_O, 0);
	if (!rd) {
		return NULL;
	}
	return rd;
}

size_t tsig_rdata_mac_length(const knot_rrset_t *tsig)
{
	uint8_t *rd = tsig_rdata_seek(tsig, TSIG_MACLEN_O, sizeof(uint16_t));
	if (!rd) {
		return 0;
	}
	return knot_wire_read_u16(rd);
}

uint16_t tsig_rdata_orig_id(const knot_rrset_t *tsig)
{
	uint8_t *rd = tsig_rdata_seek(tsig, TSIG_ORIGID_O, sizeof(uint16_t));
	if (!rd) {
		return KNOT_ERROR;
	}
	return knot_wire_read_u16(rd);
}

uint16_t tsig_rdata_error(const knot_rrset_t *tsig)
{
	uint8_t *rd = tsig_rdata_seek(tsig, TSIG_ERROR_O, sizeof(uint16_t));
	if (!rd) {
		return KNOT_ERROR;
	}
	return knot_wire_read_u16(rd);
}

const uint8_t *tsig_rdata_other_data(const knot_rrset_t *tsig)
{
	uint8_t *rd = tsig_rdata_seek(tsig, TSIG_OTHER_O, 0);
	if (!rd) {
		return NULL;
	}
	return rd;
}

uint16_t tsig_rdata_other_data_length(const knot_rrset_t *tsig)
{
	uint8_t *rd = tsig_rdata_seek(tsig, TSIG_OLEN_O, sizeof(uint16_t));
	if (!rd) {
		return KNOT_ERROR;
	}
	return knot_wire_read_u16(rd);
}

int tsig_alg_from_name(const knot_dname_t *alg_name)
{
	if (!alg_name) {
		return 0;
	}

	char *name = knot_dname_to_str(alg_name);
	if (!name) {
		return 0;
	}

	knot_lookup_table_t *found =
		knot_lookup_by_name(knot_tsig_alg_domain_names, name);

	if (!found) {
		dbg_tsig("Unknown algorithm: %s \n", name);
		free(name);
		return 0;
	}

	free(name);

	return found->id;
}

size_t tsig_rdata_tsig_variables_length(const knot_rrset_t *tsig)
{
	if (tsig == NULL) {
		return 0;
	}
	/* Key name, Algorithm name and Other data have variable lengths. */
	const knot_dname_t *key_name = knot_rrset_owner(tsig);
	if (!key_name) {
		return 0;
	}

	const knot_dname_t *alg_name = tsig_rdata_alg_name(tsig);
	if (!alg_name) {
		return 0;
	}

	uint16_t other_data_length = tsig_rdata_other_data_length(tsig);

	return knot_dname_size(key_name) + knot_dname_size(alg_name) +
	       other_data_length + KNOT_TSIG_VARIABLES_LENGTH;
}

size_t tsig_rdata_tsig_timers_length()
{
	/*! \todo Cleanup */
	return KNOT_TSIG_TIMERS_LENGTH;
}


int tsig_rdata_store_current_time(knot_rrset_t *tsig)
{
	if (!tsig) {
		return KNOT_EINVAL;
	}
	time_t curr_time = time(NULL);
	/*! \todo bleeding eyes. */
	tsig_rdata_set_time_signed(tsig, (uint64_t)curr_time);
	return KNOT_EOK;
}

const char* tsig_alg_to_str(knot_tsig_algorithm_t alg)
{
	knot_lookup_table_t *item;

	item = knot_lookup_by_id(knot_tsig_alg_domain_names, alg);

	if (item != NULL) {
		return item->name;
	} else {
		return "";
	}
}

size_t tsig_wire_maxsize(const knot_tsig_key_t *key)
{
	size_t alg_name_size = strlen(tsig_alg_to_str(key->algorithm)) + 1;

	/*! \todo Used fixed size as a base. */
	return knot_dname_size(key->name) +
	sizeof(uint16_t) + /* TYPE */
	sizeof(uint16_t) + /* CLASS */
	sizeof(uint32_t) + /* TTL */
	sizeof(uint16_t) + /* RDLENGTH */
	alg_name_size + /* Alg. name */
	6 * sizeof(uint8_t) + /* Time signed */
	sizeof(uint16_t) + /* Fudge */
	sizeof(uint16_t) + /* MAC size */
	knot_tsig_digest_length(key->algorithm) + /* MAC */
	sizeof(uint16_t) + /* Original ID */
	sizeof(uint16_t) + /* Error */
	sizeof(uint16_t) + /* Other len */
	6* sizeof(uint8_t); /* uint48_t in case of BADTIME RCODE */
}

size_t tsig_wire_actsize(const knot_rrset_t *tsig)
{
	if (tsig == NULL) {
		return 0;
	}

	/*! \todo Used fixed size as a base. */
	return knot_dname_size(knot_rrset_owner(tsig)) +
	sizeof(uint16_t) + /* TYPE */
	sizeof(uint16_t) + /* CLASS */
	sizeof(uint32_t) + /* TTL */
	sizeof(uint16_t) + /* RDLENGTH */
	knot_dname_size(tsig_rdata_alg_name(tsig)) +
	6 * sizeof(uint8_t) + /* Time signed */
	sizeof(uint16_t) + /* Fudge */
	sizeof(uint16_t) + /* MAC size */
	tsig_rdata_mac_length(tsig) +
	sizeof(uint16_t) + /* Original ID */
	sizeof(uint16_t) + /* Error */
	sizeof(uint16_t) + /* Other len */
	tsig_rdata_other_data_length(tsig);
}

int tsig_rdata_is_ok(const knot_rrset_t *tsig)
{
	/*! \todo Check size, needs to check variable-length fields. */
	return (tsig
	        && knot_rrset_get_rdata(tsig, 0) != NULL
	        && tsig_rdata_seek(tsig, TSIG_OTHER_O, 0) != NULL
	        && tsig_rdata_alg_name(tsig) != NULL
	        && tsig_rdata_time_signed(tsig) != 0);
}
