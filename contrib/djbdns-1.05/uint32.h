#ifndef UINT32_H
#define UINT32_H

#include "crypto_uint32.h"

typedef crypto_uint32 uint32;

extern void uint32_pack(char *,uint32);
extern void uint32_pack_big(char *,uint32);
extern void uint32_unpack(const char *,uint32 *);
extern void uint32_unpack_big(const char *,uint32 *);

#endif
