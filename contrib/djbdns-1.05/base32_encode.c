#include "base32.h"
#include "byte.h"

const char base32_digits[32] = "0123456789bcdfghjklmnpqrstuvwxyz";

unsigned int base32_bytessize(unsigned int len)
{
  len = (8 * len + 4) / 5;
  return len + (len + 49) / 50;
}

void base32_encodebytes(char *out,const char *in,unsigned int len)
{
  unsigned int i, x, v, vbits;

  x = v = vbits = 0;
  for (i = 0;i < len;++i) {
    v |= ((unsigned int) (unsigned char) in[i]) << vbits;
    vbits += 8;
    do {
      out[++x] = base32_digits[v & 31];
      v >>= 5;
      vbits -= 5;
      if (x == 50) {
        *out = x;
        out += 1 + x;
        x = 0;
      }
    } while (vbits >= 5);
  }

  if (vbits) out[++x] = base32_digits[v & 31];
  if (x) *out = x;
}

void base32_encodekey(char *out,const char *key)
{
  unsigned int i, v, vbits;

  byte_copy(out,4,"\66x1a");
  out += 4;

  v = vbits = 0;
  for (i = 0;i < 32;++i) {
    v |= ((unsigned int) (unsigned char) key[i]) << vbits;
    vbits += 8;
    do {
      *out++ = base32_digits[v & 31];
      v >>= 5;
      vbits -= 5;
    } while (vbits >= 5);
  }
}

