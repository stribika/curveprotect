#ifndef _SHA256_H____
#define _SHA256_H____

#include "crypto_hashblocks_sha256.h"
#include "crypto_hash_sha256.h"

#define sha256_BYTES crypto_hashblocks_sha256_STATEBYTES

typedef struct sha256_ctx {
    unsigned char h[sha256_BYTES];
    unsigned long long bytes;
} sha256_ctx;

#define sha256 crypto_hash_sha256
extern void sha256_init(sha256_ctx *ctx);
extern int sha256_block(sha256_ctx *ctx, unsigned char *in, unsigned long long inlen);
extern int sha256_last(sha256_ctx *ctx, unsigned char *h, unsigned char *in, unsigned long long inlen);


#endif
