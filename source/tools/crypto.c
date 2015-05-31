#include "crypto.h"
#include "byte.h"

int crypto_init(crypto_ctx *ctx, const unsigned char *n, const unsigned char *k, const char *magic) {

    long long i;
    int (*crypto_block_aes_beforenm)(unsigned char *,const unsigned char *) = 0;


    if (byte_isequal(magic, MAGICBYTES, MAGIC)) {
        crypto_block_aes_beforenm = crypto_block_aes256vulnerable_beforenm;
        ctx->crypto_block_aes_afternm = crypto_block_aes256vulnerable_afternm;
    }

    if (!crypto_block_aes_beforenm) return -1;

    /* aes */
    for (i = 0; i < 8; ++i) ctx->n1[i] = n[i];
    for (i = 8; i < 16; ++i) ctx->n1[i] = 0;
    crypto_block_aes_beforenm(ctx->k1, k);

    /* salsa20 */
    for (i = 0; i < 8; ++i) ctx->n2[i] = n[i + 8];
    for (i = 8; i < 16; ++i) ctx->n2[i] = 0;
    for (i = 0; i < 32; ++i) ctx->k2[i] = k[i + 32];
    return 0;
}

int crypto_last(crypto_ctx *ctx, unsigned char *xm, unsigned long long xmlen) {

    unsigned char block[64];
    unsigned int u;
    long long i;
    unsigned char *m;
    unsigned long long mlen;

    if (!xmlen) return 0;

    /* encrypt using salsa20 */
    mlen = xmlen;
    m = xm;

    while (mlen >= 64) {
        crypto_block_salsa20(block, ctx->n2, ctx->k2);
        for (i = 0; i < 64; ++i) m[i] = m[i] ^ block[i];

        u = 1;
        for (i = 8; i < 16; ++i) {
            u += (unsigned int) ctx->n2[i];
            ctx->n2[i] = u;
            u >>= 8;
        }

        mlen -= 64;
        m += 64;
    }

    if (mlen) {
        crypto_block_salsa20(block, ctx->n2, ctx->k2);
        for (i = 0; i < mlen; ++i) m[i] = m[i] ^ block[i];
    }

    /* encrypt using aes */
    mlen = xmlen;
    m = xm;

    while (mlen >= 16) {
        ctx->crypto_block_aes_afternm(block, ctx->n1, ctx->k1);
        for (i = 0; i < 16; ++i) m[i] = m[i] ^ block[i];

        u = 1;
        for (i = 15; i >= 8; --i) {
            u += (unsigned int) ctx->n1[i];
            ctx->n1[i] = u;
            u >>= 8;
        }

        mlen -= 16;
        m += 16;
    }

    if (mlen) {
        ctx->crypto_block_aes_afternm(block, ctx->n1, ctx->k1);
        for (i = 0; i < mlen; ++i) m[i] = m[i] ^ block[i];
    }

    return 0;
}

int crypto_block(crypto_ctx *ctx, unsigned char *xm, unsigned long long xmlen) {
    if (!xmlen) return 0;
    if (xmlen % 64) return -1;
    return crypto_last(ctx, xm, xmlen);
}
