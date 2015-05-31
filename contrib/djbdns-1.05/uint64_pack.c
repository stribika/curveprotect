#include "uint64.h"

void uint64_pack(char s[8],uint64 u)
{
  s[0] = u & 255;
  u >>= 8;
  s[1] = u & 255;
  u >>= 8;
  s[2] = u & 255;
  u >>= 8;
  s[3] = u & 255;
  u >>= 8;
  s[4] = u & 255;
  u >>= 8;
  s[5] = u & 255;
  u >>= 8;
  s[6] = u & 255;
  s[7] = u >> 8;
}

void uint64_pack_big(char s[8],uint64 u)
{
  s[7] = u & 255;
  u >>= 8;
  s[6] = u & 255;
  u >>= 8;
  s[5] = u & 255;
  u >>= 8;
  s[4] = u & 255;
  u >>= 8;
  s[3] = u & 255;
  u >>= 8;
  s[2] = u & 255;
  u >>= 8;
  s[1] = u & 255;
  s[0] = u >> 8;
}
