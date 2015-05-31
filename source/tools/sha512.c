#include "sha512.h"
#include "e.h"
#include "randombytes.h"
#include "crypto_uint32.h"

static unsigned char iv[sha512_BYTES] = {
  0x6a,0x09,0xe6,0x67,0xf3,0xbc,0xc9,0x08,
  0xbb,0x67,0xae,0x85,0x84,0xca,0xa7,0x3b,
  0x3c,0x6e,0xf3,0x72,0xfe,0x94,0xf8,0x2b,
  0xa5,0x4f,0xf5,0x3a,0x5f,0x1d,0x36,0xf1,
  0x51,0x0e,0x52,0x7f,0xad,0xe6,0x82,0xd1,
  0x9b,0x05,0x68,0x8c,0x2b,0x3e,0x6c,0x1f,
  0x1f,0x83,0xd9,0xab,0xfb,0x41,0xbd,0x6b,
  0x5b,0xe0,0xcd,0x19,0x13,0x7e,0x21,0x79
} ;

void sha512_init(sha512_ctx *ctx) {

    long long i;

    for (i = 0; i < crypto_hashblocks_sha512_STATEBYTES; ++i) ctx->h[i] = iv[i];
    ctx->bytes = 0;
    return;
}

int sha512_block(sha512_ctx *ctx, unsigned char *in, unsigned long long inlen) {

    if (inlen % crypto_hashblocks_sha512_BLOCKBYTES) { errno = EINVAL; return -1; }

    crypto_hashblocks_sha512(ctx->h, in, inlen);
    ctx->bytes += inlen;
    return 0;
}

int sha512_last(sha512_ctx *ctx, unsigned char *h, unsigned char *in, unsigned long long inlen) {

    long long i;
    unsigned char padded[256];

    crypto_hashblocks_sha512(ctx->h, in, inlen);
    ctx->bytes += inlen;

    in += inlen;
    inlen &= 127;
    in -= inlen;

    for (i = 0;i < inlen; ++i) padded[i] = in[i];
    padded[inlen] = 0x80;

    if (inlen < 112) {
        for (i = inlen + 1; i < 119; ++i) padded[i] = 0;
        padded[119] = ctx->bytes >> 61;
        padded[120] = ctx->bytes >> 53;
        padded[121] = ctx->bytes >> 45;
        padded[122] = ctx->bytes >> 37;
        padded[123] = ctx->bytes >> 29;
        padded[124] = ctx->bytes >> 21;
        padded[125] = ctx->bytes >> 13;
        padded[126] = ctx->bytes >> 5;
        padded[127] = ctx->bytes << 3;
        crypto_hashblocks_sha512(ctx->h, padded, 128);
    }
    else {
        for (i = inlen + 1; i < 247; ++i) padded[i] = 0;
        padded[247] = ctx->bytes >> 61;
        padded[248] = ctx->bytes >> 53;
        padded[249] = ctx->bytes >> 45;
        padded[250] = ctx->bytes >> 37;
        padded[251] = ctx->bytes >> 29;
        padded[252] = ctx->bytes >> 21;
        padded[253] = ctx->bytes >> 13;
        padded[254] = ctx->bytes >> 5;
        padded[255] = ctx->bytes << 3;
        crypto_hashblocks_sha512(ctx->h, padded, 256);
    }
    for (i = 0; i < crypto_hashblocks_sha512_STATEBYTES; ++i) h[i] = ctx->h[i];

    return 0;
}

int sha512hmac(unsigned char *out, const unsigned char *in, unsigned long long inlen, const unsigned char *kx, unsigned long long klen) {

    unsigned char h[64];
    unsigned char padded[256];
    long long i;
    unsigned long long bytes = 128 + inlen;
    unsigned char k[128];

    if (klen > 128) {
        sha512(k, kx, klen);
        klen = 64;
    }
    else {
        for (i = 0; i < klen; ++i) k[i] = kx[i];
    }

    for (i = 0; i < 64; ++i) h[i] = iv[i];

    for (i = 0;    i < klen; ++i) padded[i] = k[i] ^ 0x36;
    for (i = klen; i < 128;  ++i) padded[i] = 0x36;

    crypto_hashblocks_sha512(h, padded, 128);
    crypto_hashblocks_sha512(h, in, inlen);
    in += inlen;
    inlen &= 127;
    in -= inlen;

    for (i = 0;i < inlen;++i) padded[i] = in[i];
    padded[inlen] = 0x80;

    if (inlen < 112) {
        for (i = inlen + 1; i < 119; ++i) padded[i] = 0;
        padded[119] = bytes >> 61;
        padded[120] = bytes >> 53;
        padded[121] = bytes >> 45;
        padded[122] = bytes >> 37;
        padded[123] = bytes >> 29;
        padded[124] = bytes >> 21;
        padded[125] = bytes >> 13;
        padded[126] = bytes >> 5;
        padded[127] = bytes << 3;
        crypto_hashblocks_sha512(h, padded, 128);
    }
    else {
        for (i = inlen + 1; i < 247; ++i) padded[i] = 0;
        padded[247] = bytes >> 61;
        padded[248] = bytes >> 53;
        padded[249] = bytes >> 45;
        padded[250] = bytes >> 37;
        padded[251] = bytes >> 29;
        padded[252] = bytes >> 21;
        padded[253] = bytes >> 13;
        padded[254] = bytes >> 5;
        padded[255] = bytes << 3;
        crypto_hashblocks_sha512(h, padded, 256);
    }

    for (i = 0; i < klen; ++i) padded[i] = k[i] ^ 0x5c;
    for (i = klen; i < 128; ++i) padded[i] = 0x5c;

    for (i = 0; i < 64; ++i) padded[128 + i] = h[i];
    for (i = 0; i < 64; ++i) h[i] = iv[i];

    for (i = 64;i < 128; ++i) padded[128 + i] = 0;
    padded[128 + 64] = 0x80;
    padded[128 + 126] = 6;

    crypto_hashblocks_sha512(h, padded, 256);
    for (i = 0; i < 64; ++i) out[i] = h[i];
    return 0;
}


int sha512hmacpbkdf2(unsigned char *key, unsigned long long keylen, const unsigned char *pass, const unsigned long long passlen, const unsigned char *salt, const unsigned long long saltlen, unsigned long long rounds) {

    unsigned char asalt[128];
    unsigned char d1[64];
    unsigned char d2[64];
    unsigned char obuf[64];
    long long i, j, k;
    crypto_uint32 u;

    if (saltlen > sizeof asalt - 4) { errno = ENOMEM; return -1; }
    for (i = 0; i < saltlen; ++i) asalt[i] = salt[i];

    for (i = 0; i * 64 < keylen; ++i) {
        u = i + 1;
        asalt[saltlen + 0] = 0xff & (u >> 24);
        asalt[saltlen + 1] = 0xff & (u >> 16);
        asalt[saltlen + 2] = 0xff & (u >> 8);
        asalt[saltlen + 3] = 0xff & u;

        sha512hmac(d1, asalt, saltlen + 4, pass, passlen);
        for (j = 0; j < 64; ++j) obuf[j] = d1[j];

        for (j = 1; j < rounds; ++j) {
            sha512hmac(d2, d1, 64, pass, passlen);
            for (k = 0; k < 64; ++k) d1[k] = d2[k];
            for (k = 0; k < 64; ++k) obuf[k] ^= d1[k];
        }
        for (k = 0; k < 64; ++k) key[64 * i + k] = obuf[k];
    }
    randombytes(d1, sizeof d1);
    randombytes(d2, sizeof d2);
    randombytes(obuf, sizeof obuf);
    randombytes(asalt, sizeof asalt);
    return 0;
}
