#include "crypto_box.h"
#include "randombytes.h"
#include "dns.h"

#if crypto_box_SECRETKEYBYTES != 32
error!
#endif
#if crypto_box_PUBLICKEYBYTES != 32
error!
#endif


void dns_randomkey(unsigned char *pk, unsigned char *sk, unsigned char *nk) {

    crypto_box_keypair(pk, sk);
    randombytes(nk, 16);
}

