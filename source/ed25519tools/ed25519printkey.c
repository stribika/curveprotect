#include <unistd.h>
#include "die.h"
#include "e.h"
#include "load.h"
#include "writeall.h"
#include "crypto_sign_ed25519.h"

unsigned char pk[crypto_sign_ed25519_PUBLICKEYBYTES];
unsigned char out[crypto_sign_ed25519_PUBLICKEYBYTES * 2 + 1];

void die_usage(void)
{
  die_1(100,"ed25519printkey: usage: ed25519printkey keydir\n");
}

void die_fatal(const char *trouble,const char *d,const char *fn)
{
  if (d) {
    if (fn) die_9(111,"ed25519printkey: fatal: ",trouble," ",d,"/",fn,": ",e_str(errno),"\n");
    die_7(111,"ed25519printkey: fatal: ",trouble," ",d,": ",e_str(errno),"\n");
  }
  die_5(111,"ed25519printkey: fatal: ",trouble,": ",e_str(errno),"\n");
}

int main(int argc,char **argv)
{
  char *d;
  long long j;

  if (!argv[0]) die_usage();
  if (!argv[1]) die_usage();
  d = argv[1];

  if (chdir(d) == -1) die_fatal("unable to chdir to directory",d,0);
  if (load("publickey",pk,sizeof pk) == -1) die_fatal("unable to read",d,"publickey");

  for (j = 0;j < crypto_sign_ed25519_PUBLICKEYBYTES;++j) {
    out[2 * j + 0] = "0123456789abcdef"[15 & (int) (pk[j] >> 4)];
    out[2 * j + 1] = "0123456789abcdef"[15 & (int) (pk[j] >> 0)];
  }
  out[2 * j] = '\n';

  if (writeall(1,out,sizeof out) == -1) die_fatal("unable to write output",0,0);

  _exit(0);
}
