#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "die.h"
#include "e.h"
#include "savesync.h"
#include "crypto_sign_ed25519.h"

void die_usage(void)
{
  die_1(100,"ed25519makekey: usage: ed25519makekey keydir\n");
}

void die_fatal(const char *trouble,const char *d,const char *fn)
{
  if (fn) die_9(111,"ed25519makekey: fatal: ",trouble," ",d,"/",fn,": ",e_str(errno),"\n");
  die_7(111,"ed25519makekey: fatal: ",trouble," ",d,": ",e_str(errno),"\n");
}

unsigned char pk[crypto_sign_ed25519_PUBLICKEYBYTES];
unsigned char sk[crypto_sign_ed25519_SECRETKEYBYTES];

void create(const char *d,const char *fn,const unsigned char *x,long long xlen)
{
  if (savesync(fn,x,xlen) == -1) die_fatal("unable to create",d,fn);
}

int main(int argc,char **argv)
{
  char *d;

  if (!argv[0]) die_usage();
  if (!argv[1]) die_usage();
  d = argv[1];

  umask(022);
  if (mkdir(d,0755) == -1) die_fatal("unable to create directory",d,0);
  if (chdir(d) == -1) die_fatal("unable to chdir to directory",d,0);
  if (mkdir(".expertsonly",0700) == -1) die_fatal("unable to create directory",d,".expertsonly");

  crypto_sign_ed25519_keypair(pk,sk);
  create(d,"publickey",pk,sizeof pk);

  umask(077);
  create(d,".expertsonly/secretkey",sk,sizeof sk);

  _exit(0);
}
