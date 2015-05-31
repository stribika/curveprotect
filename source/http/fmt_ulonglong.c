#include "fmt.h"

unsigned int fmt_ulonglong(register char *s,register unsigned long long u)
{
  register unsigned int len; register unsigned long long q;
  len = 1; q = u;
  while (q > 9) { ++len; q /= 10; }
  if (s) {
    s += len;
    do { *--s = '0' + (u % 10); u /= 10; } while(u); /* handles u == 0 */
  }
  return len;
}
