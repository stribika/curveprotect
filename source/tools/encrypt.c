/*
20130113
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include "die.h"
#include "e.h"
#include "writeall.h"
#include "nanoseconds.h"
#include "uint64_pack.h"
#include "env.h"
#include "str.h"
#include "byte.h"
#include "sha512.h"
#include "crypto.h"
#include "randombytes.h"
#include "fsyncfd.h"

#define FATAL "encrypt: fatal: "

#define USAGE "\
\n\
encrypt: usage:\n\
\n\
 name:\n\
   encrypt - encrypt byte-stream\n\
\n\
 syntax:\n\
   encrypt [options] <input >output\n\
\n\
 description:\n\
   encrypt tool encrypts input byte-stream using AES256-CTR and Salsa20\n\
   and prints encrypted byte-stream in format:\n\
   +---------+---------+----------+------------------+-----------------------+\n\
   |magic(4B)|salt(12B)|nonce(16B)|encrypted_data(xB)|encrypted_checksum(16B)|\n\
   +---------+---------+----------+------------------+-----------------------+\n\
   keys are derived from password using PBKDF2-HMAC-SHA512\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 environment:\n\
   $PASSWORD (mandatory): pass phrase\n\
\n\
 example: (part of shell script)\n\
   ...\n\
   read PASSWORD; export PASSWORD\n\
   (cd /etc; tar cf - .) | gzip -9 | encrypt > etc.tgz.tmp\n\
   mv -f etc.tgz.tmp etc.tgz.enc20\n\
   ...\n\
   decrypt < etc.tgz.enc20 | gunzip | (cd /restored-etc; tar -xf - )\n\
   unset PASSWORD\n\
   ...\n\
\n\
"

void die_usage(const char *s) {
  if (s) die_4(100, USAGE, FATAL, s, "\n");
  die_1(100, USAGE);
}

sha512_ctx shactx;
crypto_ctx ctx;

void cleanup(void) {
    randombytes((unsigned char *)&ctx, sizeof ctx);
}

static void die_fatal(const char *trouble, const char *fn) {

    cleanup();

    if (errno) {
        if (fn) die_7(111, FATAL, trouble, " ", fn, ": ", e_str(errno), "\n");
        die_5(111, FATAL, trouble, ": ", e_str(errno), "\n");
    }
    if (fn) die_5(111, FATAL, trouble, " ", fn, "\n");
    die_3(111, FATAL, trouble, "\n");
}


static unsigned long long readblock(unsigned char *buf, long long len) {

    long long r, ret = 0;

    while (len > 0) {
        r = read(0, (char *)buf, len);
        if (r < 0) die_fatal("unable to read input", 0);
        ret += r;
        if (r == 0 || r == len) break;
        buf += r; len -= r;
    }
    return ret;
}

#define BLOCK 4096
static unsigned char in[BLOCK + CHECKSUMBYTES];
static unsigned char s[SALTBYTES];
static unsigned char h[HASHBYTES];
static unsigned char n[NONCEBYTES];
static unsigned char nk[HASHBYTES];
static unsigned long long inlen;

int main(int argc, char **argv) {

    unsigned char *x;
    unsigned long long xlen;

    if (argv[0])
        if (argv[1]) {
            if (str_equal(argv[1], "-h"))
                die_usage(0);
        }

    /* get password  */
    x = (unsigned char *)env_get("PASSWORD");
    if (!x) { errno = 0; die_usage("$PASSWORD not set"); }
    xlen = str_len((char *)x);

    /* create salt */
    randombytes(s, sizeof s);

    /* derive key  */
    if (sha512hmacpbkdf2(h, sizeof h, x, xlen, s, sizeof s, ROUNDS) == -1) die_fatal("unable to derive keys", 0);
    byte_zero(x, xlen);

    /* create nonce */
    randombytes(n, sizeof n);
    uint64_pack(n, nanoseconds());
    sha512(nk, (unsigned char *)MAGIC, MAGICBYTES);
    crypto_block_aes256vulnerable(n, n, nk);

    /* initialize */
    crypto_init(&ctx, n, h, MAGIC);
    randombytes(h, sizeof h);
    sha512_init(&shactx);

    /* write header */
    if (writeall(1, MAGIC, MAGICBYTES) == -1) die_fatal("unable to write output", 0);
    if (writeall(1, s, sizeof s) == -1) die_fatal("unable to write output", 0);
    randombytes(s, sizeof s);
    if (writeall(1, n, sizeof n) == -1) die_fatal("unable to write output", 0);

    for (;;) {
        inlen = readblock(in, BLOCK);
        if (inlen != BLOCK) break;
        if (sha512_block(&shactx, in, inlen) != 0) die_fatal("unable to compute hash", 0);
        if (crypto_block(&ctx, in, inlen) != 0) die_fatal("unable to encrypt stream", 0);
        if (writeall(1, in, inlen) == -1) die_fatal("unable to write output", 0);
    }
    if (sha512_last(&shactx, h, in, inlen) != 0) die_fatal("unable to compute hash", 0);
    byte_copy(in + inlen, CHECKSUMBYTES, h);
    inlen += CHECKSUMBYTES;
    if (crypto_last(&ctx, in, inlen) != 0) die_fatal("unable to encrypt stream", 0);
    if (writeall(1, in, inlen) == -1) die_fatal("unable to write output", 0);

    if (fsyncfd(1) == -1) die_fatal("unable to write output", 0);
    cleanup();
    _exit(0);
}
