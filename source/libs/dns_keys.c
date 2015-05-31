#include "crypto_box.h"
#include "randombytes.h"
#include "dns.h"

void dns_keys(unsigned char *pk, unsigned char *sk, unsigned char *nk) {

    crypto_box_keypair(pk, sk);
    randombytes(nk, 16);
}

