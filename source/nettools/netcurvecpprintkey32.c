#include <unistd.h>
#include "die.h"
#include "e.h"
#include "load.h"
#include "writeall.h"
#include "crypto_box.h"
#include "base32encode.h"

unsigned char pk[crypto_box_PUBLICKEYBYTES];
unsigned char out[((crypto_box_PUBLICKEYBYTES * 8) + 4) / 5];

void die_usage(void)
{
  die_1(111,"netcurvecpprintkey32: usage: netcurvecpprintkey32 keydir\n");
}

void die_fatal(const char *trouble,const char *d,const char *fn)
{
  if (d) {
    if (fn) die_9(111,"netcurvecpprintkey32: fatal: ",trouble," ",d,"/",fn,": ",e_str(errno),"\n");
    die_7(111,"netcurvecpprintkey32: fatal: ",trouble," ",d,": ",e_str(errno),"\n");
  }
  die_5(111,"netcurvecpprintkey32: fatal: ",trouble,": ",e_str(errno),"\n");
}

int main(int argc,char **argv)
{
  char *d;

  if (!argv[0]) die_usage();
  if (!argv[1]) die_usage();
  d = argv[1];

  if (chdir(d) == -1) die_fatal("unable to chdir to directory",d,0);
  if (load("publickey",pk,sizeof pk) == -1) die_fatal("unable to read",d,"publickey");

  if (!base32encode(out, sizeof out, pk, sizeof pk)) die_fatal("unable to encode to base32", 0, 0);
  out[sizeof out - 1] = '\n';

  if (writeall(1,out,sizeof out) == -1) die_fatal("unable to write output",0,0);
  
  return 0;
}
