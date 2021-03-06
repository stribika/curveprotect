/* Public domain. */

#include "scan.h"

unsigned int scan_ulonglong(register const char *s,register unsigned long long *u)
{
  register unsigned int pos = 0;
  register unsigned long long result = 0;
  register unsigned long long c;
  while ((c = (unsigned long long) (unsigned char) (s[pos] - '0')) < 10) {
    result = result * 10 + c;
    ++pos;
  }
  *u = result;
  return pos;
}
