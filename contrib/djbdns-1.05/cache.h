#ifndef CACHE_H
#define CACHE_H

#include "uint32.h"
#include "uint64.h"

extern uint64 cache_motion;

/* record cache stats */
/* James Raftery <james@now.ie> 6 Nov. 2003 */
extern uint64 cache_hit;
extern uint64 cache_miss;

extern int cache_init(unsigned int);
extern void cache_set(const char *,unsigned int,const char *,unsigned int,uint32,int);
extern char *cache_get(const char *,unsigned int,unsigned int *,uint32 *,int *);

extern int cache_dump(int);
extern int cache_load(void);
extern void cache_clean(int);

#endif
