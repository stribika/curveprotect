#include "base32encode.h"

static const char base32chars[] = "0123456789bcdfghjklmnpqrstuvwxyz";

int base32encode(unsigned char *out, long long outlen, const unsigned char *in, long long inlen) {

    long long i=0, j=0;
    unsigned long long v=0, bits=0;

    while (j < inlen) {
        v |= in[j++] << bits;
        bits += 8;

        while (bits >= 5) {
            if (i >= outlen) return 0;
            out[i++] = base32chars[v & 31];
            bits -= 5; v >>= 5;
        }
    }
    if (bits) {
        if (i >= outlen) return 0;
        out[i++] = base32chars[v & 31];
        bits -= 5; v >>= 5;
    }
    return 1;
}
