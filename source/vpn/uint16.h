/* Public domain. */

#ifndef _UINT16_H____
#define _UINT16_H____

#include "crypto_uint16.h"

typedef crypto_uint16 uint16;

extern void uint16_pack(char *,uint16);
extern void uint16_pack_big(char *,uint16);
extern void uint16_unpack(const char *,uint16 *);
extern void uint16_unpack_big(const char *,uint16 *);

#endif /* _UINT16_H____ */
