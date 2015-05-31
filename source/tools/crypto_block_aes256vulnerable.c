#include "rijndael.h"
#include "crypto_uint32.h"
#include "crypto_block_aes256vulnerable.h"


int crypto_block_aes256vulnerable_static_beforenm(unsigned char *c, const unsigned char *k) {

    rijndaelKeySetupEnc((crypto_uint32 *)c, k, crypto_block_aes256vulnerable_static_KEYBYTES);
    return 0;
}


int crypto_block_aes256vulnerable_static_afternm(unsigned char *out, const unsigned char *in, const unsigned char *k) {


    rijndaelEncrypt((crypto_uint32 *)k, 14, in, out);
    return 0;
}

int crypto_block_aes256vulnerable_static(unsigned char *out, const unsigned char *in, const unsigned char *k) {

    unsigned char d[crypto_block_aes256vulnerable_static_BEFORENMBYTES];
    crypto_block_aes256vulnerable_static_beforenm(d, k);
    crypto_block_aes256vulnerable_static_afternm(out, in, d);
    return 0;
}
