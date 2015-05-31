#include "uint32_unpack_big.h"

crypto_uint32 uint32_unpack_big(const unsigned char *x) {

    crypto_uint32 y;

    y  = x[0]; y <<= 8;
    y |= x[1]; y <<= 8;
    y |= x[2]; y <<= 8;
    y |= x[3];

    return y;
}
