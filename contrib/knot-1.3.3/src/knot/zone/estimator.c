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
#include <assert.h>

#include "estimator.h"
#include "dname.h"
#include "common/lists.h"
#include "libknot/zone/node.h"
#include "common/hattrie/ahtable.h"
#include "zscanner/scanner.h"
#include "common/descriptor.h"

// Constants used for tweaking, mostly malloc overhead
enum estim_consts {
	MALLOC_OVER = sizeof(size_t), // according to malloc.c, this is minimum
	DNAME_MULT = 1,
	DNAME_ADD = MALLOC_OVER * 3, // dname itself, labels, name
	RDATA_MULT = 1,
	RDATA_ADD = MALLOC_OVER, // just raw rdata, but allocation is there
	RRSET_MULT = 1,
	RRSET_ADD = MALLOC_OVER * 3, // rrset itself, rdata array, indices
	NODE_MULT = 1,
	NODE_ADD = MALLOC_OVER * 2, // node itself, rrset array
	AHTABLE_ADD = MALLOC_OVER * 3, // table, slots, slot sizes
	MALLOC_MIN = 3 * MALLOC_OVER // minimun size of malloc'd chunk, -overhead
};

typedef struct type_list_item {
	node n;
	uint16_t type;
} type_list_item_t;

typedef struct dummy_node {
	// For now only contains list of RR types
	list node_list;
} dummy_node_t;

// return: 0 not present, 1 - present
static int find_in_list(list *node_list, uint16_t type)
{
	node *n = NULL;
	WALK_LIST(n, *node_list) {
		type_list_item_t *l_entr = (type_list_item_t *)n;
		assert(l_entr);
		if (l_entr->type == type) {
			return 1;
		}
	}

	type_list_item_t *new_entry = xmalloc(sizeof(type_list_item_t));
	new_entry->type = type;

	add_head(node_list, (node *)new_entry);
	return 0;
}

// return: 0 not present (added), 1 - present
static int dummy_node_add_type(dummy_node_t *n, uint16_t t)
{
	return find_in_list(&n->node_list, t);
}

static size_t dname_memsize(const knot_dname_t *d)
{
	size_t d_size = d->size;
	size_t l_size = d->label_count;
	if (d->size < MALLOC_MIN) {
		d_size = MALLOC_MIN;
	}
	if (d->label_count < MALLOC_MIN) {
		l_size = MALLOC_MIN;
	}

	return (sizeof(knot_dname_t) + d_size + l_size)
	       * DNAME_MULT + DNAME_ADD;
}

// return: 0 - unique, 1 - duplicate
static int insert_dname_into_table(hattrie_t *table, knot_dname_t *d,
                                   dummy_node_t **n)
{
	value_t *val = hattrie_tryget(table, (char *)d->name, d->size);
	if (val == NULL) {
		// Create new dummy node to use for this dname
		*n = xmalloc(sizeof(dummy_node_t));
		init_list(&(*n)->node_list);
		*hattrie_get(table, (char *)d->name, d->size) = *n;
		return 0;
	} else {
		// Return previously found dummy node
		*n = (dummy_node_t *)(*val);
		return 1;
	}
}

// return: RDATA memsize, minus size of dnames inside
static size_t rdata_memsize(zone_estim_t *est, const scanner_t *scanner)
{
	const rdata_descriptor_t *desc = get_rdata_descriptor(scanner->r_type);
	size_t size = 0;
	for (int i = 0; desc->block_types[i] != KNOT_RDATA_WF_END; ++i) {
		// DNAME - pointer in memory
		int item = desc->block_types[i];
		if (descriptor_item_is_dname(item)) {
			size += sizeof(knot_dname_t *);
			knot_dname_t *dname =
				knot_dname_new_from_wire(scanner->r_data +
			                                 scanner->r_data_blocks[i],
			                                 scanner->r_data_blocks[i + 1] -
			                                 scanner->r_data_blocks[i],
			                                 NULL);
			if (dname == NULL) {
				return KNOT_ERROR;
			}

			knot_dname_to_lower(dname);
			dummy_node_t *n = NULL;
			if (insert_dname_into_table(est->dname_table,
			                            dname, &n) == 0) {
				// First time we see this dname, add size
				est->dname_size += dname_memsize(dname);
			}
			knot_dname_free(&dname);
		} else if (descriptor_item_is_fixed(item)) {
		// Fixed length
			size += item;
		} else {
		// Variable length
			size += scanner->r_data_blocks[i + 1] -
			        scanner->r_data_blocks[i];
		}
	}

	return size * RDATA_MULT + RDATA_ADD;
}

static void rrset_memsize(zone_estim_t *est, const scanner_t *scanner)
{
	// Handle RRSet's owner
	knot_dname_t *owner = knot_dname_new_from_wire(scanner->r_owner,
	                         scanner->r_owner_length,
	                         NULL);
	if (owner == NULL) {
		return;
	}

	dummy_node_t *n;
	if (insert_dname_into_table(est->node_table, owner, &n) == 0) {
		// First time we see this name == new node
		est->node_size += sizeof(knot_node_t) * NODE_MULT + NODE_ADD;
		// Also, RRSet's owner will now contain full dname
		est->dname_size += dname_memsize(owner);
		// Trie's nodes handled at the end of computation
	}
	knot_dname_free(&owner);
	assert(n);

	// We will always add RDATA
	size_t rdlen = rdata_memsize(est, scanner);
	if (rdlen < MALLOC_MIN) {
		rdlen = MALLOC_MIN;
	}
	// DNAME's size not included (handled inside rdata_memsize())
	est->rdata_size += rdlen;

	est->record_count++;

	/*
	 * RDATA size done, now add static part of RRSet to size.
	 * Do not add for RRs that would be merged.
	 * All possible duplicates will be added to total size.
	 */

	if (dummy_node_add_type(n, scanner->r_type) == 0) {
		/*
		 * New RR type, add actual RRSet struct's size:
		 * MALLOC_MIN is added because of index array - usually not many RRs
		 * are in the RRSet and values in the array are type uint32, so
		 * 3 would be needed on 32bit system and 6 on 32bit system in order to
		 * be larger than MALLOC_MIN, so we use it instead. Of course, if there
		 * are more than 3/6 records in RRSet, measurement will not be precise.
		 */
		est->rrset_size += (sizeof(knot_rrset_t) + MALLOC_MIN)
		                   * RRSET_MULT + RRSET_ADD;
		// Add pointer in node's array
		est->node_size += sizeof(knot_rrset_t *);
	} else {
		// Merge would happen, so just RDATA index is added
		//est->rrset_size += sizeof(uint32_t);
	}
}

void *estimator_malloc(void *ctx, size_t len)
{
	size_t *count = (size_t *)ctx;
	*count += len + MALLOC_OVER;
	return xmalloc(len);
}

void estimator_free(void *p)
{
	free(p);
}

static void get_ahtable_size(void *t, void *d)
{
	ahtable_t *table = (ahtable_t *)t;
	size_t *size = (size_t *)d;
	// info about allocated chunks starts at table->n-th index
	for (size_t i = table->n; i < table->n * 2; ++i) {
		// add actual slot size (= allocated for slot)
		*size += table->slot_sizes[i];
		// each non-empty slot means allocation overhead
		*size += table->slot_sizes[i] ? MALLOC_OVER : 0;
	}
	*size += sizeof(ahtable_t);
	// slot sizes + allocated sizes
	*size += (table->n * 2) * sizeof(uint32_t);
	// slots
	*size += table->n * sizeof(void *);
	*size += AHTABLE_ADD;
}

size_t estimator_trie_ahtable_memsize(hattrie_t *table)
{
	/*
	 * Iterate through trie's node, and get stats from each ahtable.
	 * Space taken up by the trie itself is measured using malloc wrapper.
	 * (Even for large zones, space taken by trie itself is very small)
	 */
	size_t size = 0;
	hattrie_apply_rev_ahtable(table, get_ahtable_size, &size);
	return size;
}

void estimator_rrset_memsize_wrap(const scanner_t *scanner)
{
	rrset_memsize(scanner->data, scanner);
}

void estimator_free_trie_node(value_t *val, void *data)
{
	UNUSED(data);
	dummy_node_t *trie_n = (dummy_node_t *)(*val);
	WALK_LIST_FREE(trie_n->node_list);
	free(trie_n);
}
