#include "crypto_block_salsa20.h"

static const unsigned char sigma[16] = "expand 32-byte k";

int crypto_block_salsa20_static(unsigned char *out, const unsigned char *in, const unsigned char *k) {

    return crypto_core_salsa20(out, in, k, sigma);
}
