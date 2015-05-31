#include "uint64.h"

void uint64_unpack(const char s[8],uint64 *u)
{
  uint64 result;

  result = (unsigned char) s[7];
  result <<= 8;
  result += (unsigned char) s[6];
  result <<= 8;
  result += (unsigned char) s[5];
  result <<= 8;
  result += (unsigned char) s[4];
  result <<= 8;
  result += (unsigned char) s[3];
  result <<= 8;
  result += (unsigned char) s[2];
  result <<= 8;
  result += (unsigned char) s[1];
  result <<= 8;
  result += (unsigned char) s[0];

  *u = result;
}
