/*
 * This file is part of hat-trie
 *
 * Copyright (c) 2011 by Daniel C. Jones <dcjones@cs.washington.edu>
 *
 *
 * This is an implementation of the HAT-trie data structure described in,
 *
 *    Askitis, N., & Sinha, R. (2007). HAT-trie: a cache-conscious trie-based data
 *    structure for strings. Proceedings of the thirtieth Australasian conference on
 *    Computer science-Volume 62 (pp. 97–105). Australian Computer Society, Inc.
 *
 * The HAT-trie is in essence a hybrid data structure, combining tries and hash
 * tables in a clever way to try to get the best of both worlds.
 *
 */

#ifndef HATTRIE_HATTRIE_H
#define HATTRIE_HATTRIE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>
#include "common.h"
#include "common/mempattern.h"

/* Hat-trie defines. */
typedef void* value_t;         /* User pointers as value. */
#define AHTABLE_INIT_SIZE 1024
#define TRIE_ZEROBUCKETS  0    /* Do not use hash buckets (pure trie). */
#define TRIE_BUCKET_SIZE  1536 /* Reasonably low for ordered search perf. */
#define TRIE_MAXCHAR      0xff /* Use 7-bit ASCII alphabet. */

typedef struct hattrie_t_ hattrie_t;

hattrie_t* hattrie_create (void);             //< Create an empty hat-trie.
void       hattrie_free   (hattrie_t*);       //< Free all memory used by a trie.
void       hattrie_clear  (hattrie_t*);       //< Remove all entries.
size_t     hattrie_weight (hattrie_t*);       //< Number of entries

/** Create new trie with custom bucket size and memory management.
 */
hattrie_t* hattrie_create_n (unsigned, const mm_ctx_t *);

/** Duplicate an existing trie.
 */
hattrie_t* hattrie_dup (const hattrie_t*, value_t (*nval)(value_t));

/** Build order index on all ahtable nodes in trie.
 */
void hattrie_build_index (hattrie_t*);

void hattrie_apply_rev (hattrie_t*, void (*f)(value_t*,void*), void* d);
void hattrie_apply_rev_ahtable(hattrie_t* T, void (*f)(void*,void*), void* d);

/** Find the given key in the trie, inserting it if it does not exist, and
 * returning a pointer to it's key.
 *
 * This pointer is not guaranteed to be valid after additional calls to
 * hattrie_get, hattrie_del, hattrie_clear, or other functions that modifies the
 * trie.
 */
value_t* hattrie_get (hattrie_t*, const char* key, size_t len);

/** Find a given key in the table, returning a NULL pointer if it does not
 * exist. */
value_t* hattrie_tryget (hattrie_t*, const char* key, size_t len);

/** Find a given key in the table, returning a NULL pointer if it does not
 * exist. Also set prev to point to previous node. */
int hattrie_find_leq (hattrie_t*, const char* key, size_t len, value_t** dst);

/** Find a longest prefix match. */
int hattrie_find_lpr (hattrie_t*, const char* key, size_t len, value_t** dst);


/** Delete a given key from trie. Returns 0 if successful or -1 if not found.
 */
int hattrie_del(hattrie_t* T, const char* key, size_t len);

typedef struct hattrie_iter_t_ hattrie_iter_t;

hattrie_iter_t* hattrie_iter_begin     (const hattrie_t*, bool sorted);
void            hattrie_iter_next      (hattrie_iter_t*);
bool            hattrie_iter_finished  (hattrie_iter_t*);
void            hattrie_iter_free      (hattrie_iter_t*);
const char*     hattrie_iter_key       (hattrie_iter_t*, size_t* len);
value_t*        hattrie_iter_val       (hattrie_iter_t*);

#ifdef __cplusplus
}
#endif

#endif
