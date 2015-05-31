#ifndef _SHA512_H____
#define _SHA512_H____

#include "crypto_hashblocks_sha512.h"
#include "crypto_hash_sha512.h"

#define sha512_BYTES crypto_hashblocks_sha512_STATEBYTES

typedef struct sha512_ctx {
    unsigned char h[sha512_BYTES];
    unsigned long long bytes;
} sha512_ctx;

#define sha512 crypto_hash_sha512
extern void sha512_init(sha512_ctx *ctx);
extern int sha512_block(sha512_ctx *ctx, unsigned char *in, unsigned long long inlen);
extern int sha512_last(sha512_ctx *ctx, unsigned char *h, unsigned char *in, unsigned long long inlen);

extern int sha512hmac(unsigned char *out, const unsigned char *in, unsigned long long inlen, const unsigned char *k, unsigned long long klen);
extern int sha512hmacpbkdf2(unsigned char *key, unsigned long long keylen, const unsigned char *pass, const unsigned long long passlen, const unsigned char *salt, const unsigned long long saltlen, unsigned long long rounds);


#endif
