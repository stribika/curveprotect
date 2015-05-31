/* Public domain. */

#ifndef _UINT64_H____
#define _UINT64_H____

#include "crypto_uint64.h"

typedef crypto_uint64 uint64;

extern void uint64_pack(char *,uint64);
extern void uint64_pack_big(char *,uint64);
extern void uint64_unpack(const char *,uint64 *);
extern void uint64_unpack_big(const char *,uint64 *);

#endif /* _UINT64_H____ */
