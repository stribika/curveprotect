#ifndef _crypto_block_salsa20_h____
#define _crypto_block_salsa20_h____

#include "crypto_core_salsa20.h"

#define crypto_block_salsa20_static_KEYBYTES crypto_core_salsa20_KEYBYTES
#define crypto_block_salsa20_static_BLOCKBYTES crypto_core_salsa20_OUTPUTBYTES

extern int crypto_block_salsa20_static(unsigned char *out, const unsigned char *in, const unsigned char *k);

#define crypto_block_salsa20 crypto_block_salsa20_static
#define crypto_block_salsa20_IMPLEMENTATION "static"

#define crypto_block_salsa20_KEYBYTES crypto_block_salsa20_static_KEYBYTES
#define crypto_block_salsa20_BLOCKBYTES crypto_block_salsa20_static_BLOCKBYTES

#endif
