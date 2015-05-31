#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

#include "crypto_sign_ed25519.h"
#include "writeall.h"
#include "die.h"
#include "e.h"
#include "load.h"
#include "byte.h"

unsigned char pk[crypto_sign_ed25519_PUBLICKEYBYTES];
unsigned long long mlen;
unsigned long long smlen;
unsigned long long smalen;
unsigned char *m;
unsigned char *sm;


void die_usage(void)
{
  die_1(100,"ed25519signopen: usage: ed25519signopen keydir\n");
}


void die_fatal(const char *trouble,const char *d,const char *fn)
{
  if (d) {
    if (fn) die_9(111,"ed25519signopen: fatal: ",trouble," ",d,"/",fn,": ",e_str(errno),"\n");
    die_7(111,"ed25519signopen: fatal: ",trouble," ",d,": ",e_str(errno),"\n");
  }
  die_5(111,"ed25519signopen: fatal: ",trouble,": ",e_str(errno),"\n");
}

static unsigned char *mymalloc(long long len) {

    char *x;
    unsigned long long l = len;

    l = len;
    if (l > 1073741824) {errno = ENOMEM; die_fatal("unable to allocate memory", 0, 0);}

    x = malloc(l);
    if (!x) die_fatal("unable to allocate memory", 0, 0);

    return (unsigned char *)x;
}


static void mywriteall(unsigned char *buf, long long len) {

    if (writeall(1, buf, len) == -1) die_fatal("unable to write output", 0, 0);
}


static long long myread(unsigned char *buf, long long len) {

    long long r;

    if (len > 1048576) len = 1048576;

    r = read(0, (char *)buf, len);
    if (r == -1) die_fatal("unable to read input", 0, 0);
    return r;
}




int main(int argc, char **argv) {
    
    struct stat st;
    const char *keydir;
    unsigned char *newsm;
    long long r;

    if (!argv[0]) die_usage();
    if (!argv[1]) die_usage();
    keydir = argv[1];

    if (chdir(keydir) == -1) die_fatal("unable to chdir to",keydir,0);
    if (load("publickey", pk, sizeof pk) == -1) die_fatal("unable to read public key from",keydir,0); 

    if (fstat(0,&st) == 0) {
        sm = (unsigned char *)mmap(0,st.st_size,PROT_READ,MAP_SHARED,0,0);
        if (sm != MAP_FAILED) {
            smlen = st.st_size;
            goto finish;
        }
    }

    sm = (unsigned char *)0;
    smlen = 0;
    smalen = 0;

    for(;;) {
        if (smlen + 1 > smalen) {
            smalen = 8192 + 2 * smalen ;
            newsm = mymalloc(smalen * sizeof (unsigned char));
            if (sm) {
                byte_copy(newsm, smlen * sizeof (unsigned char), sm);
                free(sm);
            }
            sm = newsm;
        }
        r = myread(sm + smlen, smalen - smlen);
        if (r == 0) break;
        smlen += r;
    }


finish:

    m = mymalloc(smlen);
    if (crypto_sign_ed25519_open(m,&mlen,sm,smlen,pk) == 0) { mywriteall(m,mlen); _exit(0); }
    errno = EINVAL;
    die_fatal("bad signature",0,0); 
    return 111;
}

