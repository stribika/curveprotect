#ifndef UINT16_H
#define UINT16_H

#include "crypto_uint16.h"

typedef crypto_uint16 uint16;

extern void uint16_pack(char *,uint16);
extern void uint16_pack_big(char *,uint16);
extern void uint16_unpack(const char *,uint16 *);
extern void uint16_unpack_big(const char *,uint16 *);

#endif
