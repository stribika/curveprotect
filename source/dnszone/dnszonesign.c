#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include "alloc.h"
#include "crypto_sign_ed25519.h"
#include "writeall.h"
#include "hexparse.h"
#include "die.h"
#include "e.h"
#include "byte.h"
#include "env.h"
#include "buffer.h"

#define FATAL "dnszonesign: fatal: "

void die_usage(void) {
    die_1(100, "dnszonesign: usage: dnszonesign <input >output\n");
}

void die_fatal(const char *trouble, const char *d, const char *fn) {

    if (d) {
        if (fn) die_9(111,FATAL,trouble," ",d,"/",fn,": ",e_str(errno),"\n");
        die_7(111,FATAL,trouble," ",d,": ",e_str(errno),"\n");
    }
    die_5(111,FATAL,trouble,": ",e_str(errno),"\n");
}

static unsigned char *mymalloc(long long len) {

    char *x;
    unsigned long long l = len;

    l = len;
    if (l > 1073741824) {errno = ENOMEM; die_fatal("unable to allocate memory", 0, 0);}

    x = alloc(l);
    if (!x) die_fatal("unable to allocate memory", 0, 0);

    return (unsigned char *)x;
}

static void mywriteall(unsigned char *buf, long long len) {

    struct stat st;

    if (writeall(1, buf, len) == -1) die_fatal("unable to write output", 0, 0);

    if (fstat(1, &st) == 0) {
        if (!S_ISREG(st.st_mode)) return;
        if (fsync(1) == -1) die_fatal("unable to write output", 0, 0);
        if (close(1) == -1) die_fatal("unable to write output", 0, 0);
    }
    return;
}


static long long get(unsigned char *buf, long long len) {

    long long r;

    r = buffer_get(buffer_0, (char *)buf, len);
    if (r == -1) die_fatal("unable to read input", 0, 0);
    return r;
}


unsigned char sk[crypto_sign_ed25519_SECRETKEYBYTES];
unsigned long long mlen;
unsigned long long malen;
unsigned long long smlen;
unsigned char *m;
unsigned char *sm;


int main(int argc, char **argv) {
    
    const char *x;
    unsigned char *newm;
    long long r;
    struct stat st;

    x = env_get("SECRETSIGNKEY");
    if (!x) die_fatal("$SECRETSIGNKEY not set", 0, 0);
    if (!hexparse(sk, crypto_sign_ed25519_SECRETKEYBYTES, x)) die_fatal("$SECRETSIGNKEY must be exactly 128 hex characters", 0, 0);

    if (fstat(0, &st) == 0) {
        m = (unsigned char *)mmap(0, st.st_size, PROT_READ, MAP_SHARED, 0, 0);
        if (m != MAP_FAILED) {
            mlen = st.st_size;
            goto finish;
        }
    }

    m = (unsigned char *)0;
    mlen = 0;
    malen = 0;

    for (;;) {
        if (mlen + 1 > malen) {
            malen = 8192 + 2 * malen ;
            newm = mymalloc(malen * sizeof (unsigned char));
            if (m) {
                byte_copy(newm, mlen * sizeof (unsigned char), m);
                alloc_free(m);
            }
            m = newm;
        }
        r = get(m + mlen, malen - mlen);
        if (r == 0) break;
        mlen += r;
    }

finish:
    sm = mymalloc(mlen * sizeof (unsigned char) + crypto_sign_ed25519_BYTES);

    crypto_sign_ed25519(sm, &smlen, m, mlen, sk);
    mywriteall(sm, smlen);
    _exit(0);
}
