#include "byte.h"
#include "dns.h"

/* XXX: sort servers by configurable notion of closeness? */
/* XXX: pay attention to competence of each server? */

void dns_sortip(char *s,unsigned int n)
{
  unsigned int i;
  char tmp[4];

  n >>= 2;
  while (n > 1) {
    i = dns_random(n);
    --n;
    byte_copy(tmp,4,s + (i << 2));
    byte_copy(s + (i << 2),4,s + (n << 2));
    byte_copy(s + (n << 2),4,tmp);
  }
}

void dns_sortip2(char *s,char *t,unsigned int n)
{
  unsigned int i,j,nn = n;
  char tmp[33];
  char *kk;

  while (n > 1) {
    i = dns_random(n);
    --n;
    byte_copy(tmp,4,s + (i << 2));
    byte_copy(s + (i << 2),4,s + (n << 2));
    byte_copy(s + (n << 2),4,tmp);
    byte_copy(tmp,33,t + 33 * i);
    byte_copy(t + 33 * i,33,t + 33 * n);
    byte_copy(t + 33 * n,33,tmp);
  }

  n = nn;
  j = 0;
  for (i = 0; i < n; ++i) {
    kk = t + 33 * i;
    if (kk[0] == 1) {
      byte_copy(tmp,4,s + (j << 2));
      byte_copy(s + (j << 2),4,s + (i << 2));
      byte_copy(s + (i << 2),4,tmp);
      byte_copy(tmp,33,t + 33 * j);
      byte_copy(t + 33 * j,33,t + 33 * i);
      byte_copy(t + 33 * i,33,tmp);
      ++j;
    }
  }

  n = nn;
  for (i = 0; i < n; ++i) {
    kk = t + 33 * i;
    if (kk[0] == 2) {
      byte_copy(tmp,4,s + (j << 2));
      byte_copy(s + (j << 2),4,s + (i << 2));
      byte_copy(s + (i << 2),4,tmp);
      byte_copy(tmp,33,t + 33 * j);
      byte_copy(t + 33 * j,33,t + 33 * i);
      byte_copy(t + 33 * i,33,tmp);
      ++j;
    }
  }
}
