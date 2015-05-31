#ifndef _CRYPTO_H____
#define _CRYPTO_H____

#include "crypto_block_aes256vulnerable.h"
#include "crypto_block_salsa20.h"
#include "crypto_hash_sha512.h"

#define SALTBYTES 12
#define ROUNDS 20000

#define HASHBYTES crypto_hash_sha512_BYTES
#define NONCEBYTES 16
#define CHECKSUMBYTES 16

#define MAGIC "5sA2"
#define MAGICBYTES (sizeof(MAGIC) - 1)

#define BEFORENMBYTES crypto_block_aes256vulnerable_BEFORENMBYTES

typedef struct crypto_ctx {
    unsigned char n1[16];
    unsigned char n2[16];
    unsigned char k1[BEFORENMBYTES];
    unsigned char k2[crypto_block_salsa20_KEYBYTES];
    int (*crypto_block_aes_afternm)(unsigned char *out, const unsigned char *in, const unsigned char *k);
} crypto_ctx;

extern int crypto_init(crypto_ctx *ctx, const unsigned char *n, const unsigned char *k, const char *magic);
extern int crypto_last(crypto_ctx *ctx, unsigned char *xm, unsigned long long xmlen);
extern int crypto_block(crypto_ctx *ctx, unsigned char *xm, unsigned long long xmlen);


#endif
