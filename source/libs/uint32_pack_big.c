#include "uint32_pack_big.h"

void uint32_pack_big(unsigned char *y, crypto_uint32 x) {

    y[3] = x & 255; x >>= 8;
    y[2] = x & 255; x >>= 8;
    y[1] = x & 255; x >>= 8;
    y[0] = x & 255;
}
