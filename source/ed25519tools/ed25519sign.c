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

unsigned char sk[crypto_sign_ed25519_SECRETKEYBYTES];
unsigned long long mlen;
unsigned long long malen;
unsigned long long smlen;
unsigned char *m;
unsigned char *sm;

void die_usage(void)
{
  die_1(100,"ed25519sign: usage: ed25519sign keydir\n");
}

void die_fatal(const char *trouble,const char *d,const char *fn)
{
  if (d) {
    if (fn) die_9(111,"ed25519sign: fatal: ",trouble," ",d,"/",fn,": ",e_str(errno),"\n");
    die_7(111,"ed25519sign: fatal: ",trouble," ",d,": ",e_str(errno),"\n");
  }
  die_5(111,"ed25519sign: fatal: ",trouble,": ",e_str(errno),"\n");
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
    unsigned char *newm;
    long long r;

    if (!argv[0]) die_usage();
    if (!argv[1]) die_usage();
    keydir = argv[1];

    if (chdir(keydir) == -1) die_fatal("unable to chdir to",keydir,0);
    if (load(".expertsonly/secretkey", sk, sizeof sk) == -1) die_fatal("unable to read secret key from",keydir,0);

    if (fstat(0,&st) == 0) {
        m = (unsigned char *)mmap(0,st.st_size,PROT_READ,MAP_SHARED,0,0);
        if (m != MAP_FAILED) {
            mlen = st.st_size;
            goto finish;
        }
    }

    m = (unsigned char *)0;
    mlen = 0;
    malen = 0;

    for(;;) {
        if (mlen + 1 > malen) {
            malen = 8192 + 2 * malen ;
            newm = mymalloc(malen * sizeof (unsigned char));
            if (m) {
                byte_copy(newm, mlen * sizeof (unsigned char), m);
                free(m);
            }
            m = newm;
        }
        r = myread(m + mlen, malen - mlen);
        if (r == 0) break;
        mlen += r;
    }

finish:
    sm = mymalloc(mlen + crypto_sign_ed25519_BYTES);
    crypto_sign_ed25519(sm,&smlen,m,mlen,sk);
    mywriteall(sm,smlen);
    _exit(0);
}

