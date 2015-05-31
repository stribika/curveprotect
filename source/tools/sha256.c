#include "sha256.h"
#include "e.h"

static unsigned char iv[sha256_BYTES] = {
  0x6a,0x09,0xe6,0x67,
  0xbb,0x67,0xae,0x85,
  0x3c,0x6e,0xf3,0x72,
  0xa5,0x4f,0xf5,0x3a,
  0x51,0x0e,0x52,0x7f,
  0x9b,0x05,0x68,0x8c,
  0x1f,0x83,0xd9,0xab,
  0x5b,0xe0,0xcd,0x19,
} ;

void sha256_init(sha256_ctx *ctx) {

    long long i;

    for (i = 0; i < crypto_hashblocks_sha256_STATEBYTES; ++i) ctx->h[i] = iv[i];
    ctx->bytes = 0;
    return;
}

int sha256_block(sha256_ctx *ctx, unsigned char *in, unsigned long long inlen) {

    if (inlen % crypto_hashblocks_sha256_BLOCKBYTES) { errno = EINVAL; return -1; }

    crypto_hashblocks_sha256(ctx->h, in, inlen);
    ctx->bytes += inlen;
    return 0;
}

int sha256_last(sha256_ctx *ctx, unsigned char *h, unsigned char *in, unsigned long long inlen) {

    long long i;
    unsigned char padded[128];
    unsigned long long bits;

    crypto_hashblocks_sha256(ctx->h, in, inlen);
    ctx->bytes += inlen;

    bits = ctx->bytes << 3;

    in += inlen;
    inlen &= 63;
    in -= inlen;

    for (i = 0; i < inlen; ++i) padded[i] = in[i];
    padded[inlen] = 0x80;

    if (inlen < 56) {
        for (i = inlen + 1; i < 56; ++i) padded[i] = 0;
        padded[56] = bits >> 56;
        padded[57] = bits >> 48;
        padded[58] = bits >> 40;
        padded[59] = bits >> 32;
        padded[60] = bits >> 24;
        padded[61] = bits >> 16;
        padded[62] = bits >> 8;
        padded[63] = bits;
        crypto_hashblocks_sha256(ctx->h, padded, 64);
    }
    else {
        for (i = inlen + 1; i < 120; ++i) padded[i] = 0;
        padded[120] = bits >> 56;
        padded[121] = bits >> 48;
        padded[122] = bits >> 40;
        padded[123] = bits >> 32;
        padded[124] = bits >> 24;
        padded[125] = bits >> 16;
        padded[126] = bits >> 8;
        padded[127] = bits;
        crypto_hashblocks_sha256(ctx->h, padded, 128);
    }
    for (i = 0; i < crypto_hashblocks_sha256_STATEBYTES; ++i) h[i] = ctx->h[i];

    return 0;
}
