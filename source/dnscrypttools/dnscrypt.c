#include "byte.h"
#include "uint32_pack_big.h"
#include "uint32_unpack_big.h"
#include "uint16_pack_big.h"
#include "uint16_unpack_big.h"
#include "dnscrypt.h"

void dnscrypt_pack(unsigned char *sm, unsigned char *providersk, unsigned char *serverpk, crypto_uint32 serial, crypto_uint32 periodsince, crypto_uint32 priodto) {

    unsigned char m[DNSCRYPT_LEN];
    unsigned long long smlen;
    unsigned char *p;

    p = m;
    byte_copy(p, 32, serverpk); p += 32;
    byte_copy(p, 8, DNSCRYPT_QUERY_MAGIC); p += 8;
    uint32_pack_big(p, serial); p += 4;
    uint32_pack_big(p, periodsince); p += 4;
    uint32_pack_big(p, priodto); p += 4;

    p = sm;
    byte_copy(p, 4, DNSCRYPT_MAGIC); p += 4;
    uint16_pack_big(p, DNSCRYPT_MAJOR); p += 2;
    uint16_pack_big(p, DNSCRYPT_MINOR); p += 2;
    dnscrypt_sign(sm + 8, &smlen, m, DNSCRYPT_LEN - 8 - crypto_sign_ed25519_BYTES, providersk);
    return;
}

const char *dnscrypt_unpack(unsigned char *serverpk, char *query_magic, crypto_uint32 *serial, crypto_uint32 *periodsince, crypto_uint32 *periodto, unsigned char *providerpk, unsigned char *sm, long long len) {

    crypto_uint16 num;
    unsigned char m[DNSCRYPT_LEN];
    unsigned long long mlen;

    if (len < DNSCRYPT_LEN) return "record too short";
    if (len > DNSCRYPT_LEN) return "record too long";
    if (byte_diff(sm, 4, DNSCRYPT_MAGIC)) return "bad magic";
    num = uint16_unpack_big(sm + 4);
    if (num != DNSCRYPT_MAJOR) return "bad version";
    num = uint16_unpack_big(sm + 6);
    if (num != DNSCRYPT_MINOR) return "bad version";
    if (dnscrypt_sign_open(m, &mlen, sm + 8, len - 8, providerpk) != 0) return "bad signature";
    byte_copy(serverpk, 32, m);
    byte_copy(query_magic, 8, m + 32);
    *serial = uint32_unpack_big(m + 40);
    *periodsince = uint32_unpack_big(m + 44);
    *periodto = uint32_unpack_big(m + 48);
    return 0;
}
