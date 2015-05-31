#ifndef _DNSCRYPT_H____
#define _DNSCRYPT_H____

#include "crypto_uint16.h"
#include "crypto_uint32.h"
#include "crypto_sign_ed25519.h"
#include "crypto_box_curve25519xsalsa20poly1305.h"

#define DNSCRYPT_LEN 4 + 2 + 2 + crypto_sign_ed25519_BYTES + crypto_box_curve25519xsalsa20poly1305_PUBLICKEYBYTES + 8 + 4 + 4 + 4
#define DNSCRYPT_QUERY_MAGIC "q6fnvWj9"
#define DNSCRYPT_RESPONSE_MAGIC "r6fnvWj8"
#define DNSCRYPT_MAGIC "DNSC"
#define DNSCRYPT_MAJOR 1
#define DNSCRYPT_MINOR 0

#define DNSCRYPT_BLOCK 64

#define dnscrypt_sign crypto_sign_ed25519
#define dnscrypt_sign_open crypto_sign_ed25519_open

extern void dnscrypt_pack(unsigned char *sm, unsigned char *providersk, unsigned char *serverpk, crypto_uint32 serial,  crypto_uint32 periodsince, crypto_uint32 priodto);
extern const char *dnscrypt_unpack(unsigned char *serverpk, char *query_magic, crypto_uint32 *serial, crypto_uint32 *periodsince, crypto_uint32 *periodto, unsigned char *providerpk, unsigned char *sm, long long len);

#endif
