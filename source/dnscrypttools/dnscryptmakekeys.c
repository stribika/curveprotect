/*
20130106
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include "e.h"
#include "die.h"
#include "buffer.h"
#include "randombytes.h"
#include "str.h"
#include "fsyncfd.h"

#include "crypto_sign_ed25519.h"
#include "crypto_box_curve25519xsalsa20poly1305.h"

#define FATAL "dnscryptmakekeys: fatal: "

#define USAGE "\
\n\
dnscryptmakekeys: usage:\n\
\n\
 name:\n\
   dnscryptmakekeys - make DNSCrypt keys\n\
\n\
 syntax:\n\
   dnscryptmakekeys [options]\n\
\n\
 description:\n\
   dnscryptmakekeys creates DH and signing key-pairs\n\
   and prints it in hexadecimal format\n\
\n\
 options:\n\
   -q (optional): no error messages\n\
   -Q (optional): print error messages (default)\n\
   -v (optional): print extra information\n\
   -h (optional): print this help\n\
   -e (optional): make only Ed25519 signing key\n\
   -c (optional): make only Curve25519 DH key\n\
   -b (optional): make both keys (default)\n\
\n\
 example:\n\
   dnscryptmakekeys\n\
\
"

static int flagverbose = 1;

void die_usage(const char *s) {
    if (!flagverbose) die_0(100);
    if (s) die_4(100, USAGE, FATAL, s, "\n");
    die_1(100, USAGE);
}

static void die_fatal(const char *trouble, const char *fn) {

    if (errno) {
        if (fn) die_7(111, FATAL, trouble, " ", fn, ": ", e_str(errno), "\n");
        die_5(111, FATAL, trouble, ": ", e_str(errno), "\n");
    }
    if (fn) die_5(111, FATAL, trouble, " ", fn, "\n");
    die_3(111, FATAL, trouble, "\n");
}

static void out(const char *x, long long len) {
    if (buffer_put(buffer_1, x, len) == -1) die_fatal("unable to write output", 0);
}

static void outs(const char *x) {
    if (buffer_puts(buffer_1, x) == -1) die_fatal("unable to write output", 0);
}

static void flush(void) {
    if (buffer_flush(buffer_1) == -1) die_fatal("unable to write output", 0);
}

static int flagdhkey = 1;
static int flagsignkey = 1;

static unsigned char pk[crypto_sign_ed25519_PUBLICKEYBYTES];
static unsigned char sk[crypto_sign_ed25519_SECRETKEYBYTES];
static unsigned char nk[16];

int main(int argc, char **argv) {

    long long i;
    char ch;
    char *x;

    if (!argv[0]) die_usage(0);
    for (;;) {
        if (!argv[1]) break;
        if (argv[1][0] != '-') break;
        x = *++argv;
        if (x[0] == '-' && x[1] == 0) break;
        if (x[0] == '-' && x[1] == '-' && x[2] == 0) break;
        while (*++x) {
            if (*x == 'q') { flagverbose = 0; continue; }
            if (*x == 'Q') { flagverbose = 1; continue; }
            if (*x == 'v') { if (flagverbose == 2) flagverbose = 3; else flagverbose = 2; continue; }
            if (*x == 'h') die_usage(0);
            if (*x == 'e') { flagsignkey = 1; flagdhkey = 0; continue; }
            if (*x == 'c') { flagsignkey = 0; flagdhkey = 1; continue; }
            if (*x == 'b') { flagsignkey = 1; flagdhkey = 1; continue; }
            die_usage(0);
        }
    }

    if (flagdhkey) {
        crypto_box_curve25519xsalsa20poly1305_keypair(pk, sk);
        randombytes(nk, sizeof nk);
        outs("Curve25519 pk: ");
        for (i = 0; i < crypto_box_curve25519xsalsa20poly1305_PUBLICKEYBYTES; ++i) {
            ch = "0123456789abcdef"[15 & (int) (pk[i] >> 4)]; out(&ch, 1);
            ch = "0123456789abcdef"[15 & (int) (pk[i] >> 0)]; out(&ch, 1);
        }
        outs("\n");
        outs("Curve25519 sk: ");
        for (i = 0; i < crypto_box_curve25519xsalsa20poly1305_SECRETKEYBYTES; ++i) {
            ch = "0123456789abcdef"[15 & (int) (sk[i] >> 4)]; out(&ch, 1);
            ch = "0123456789abcdef"[15 & (int) (sk[i] >> 0)]; out(&ch, 1);
        }
        outs("\n");
        outs("Nonce key: ");
        for (i = 0; i < 16; ++i) {
            ch = "0123456789abcdef"[15 & (int) (nk[i] >> 4)]; out(&ch, 1);
            ch = "0123456789abcdef"[15 & (int) (nk[i] >> 0)]; out(&ch, 1);
        }
        outs("\n");
    }

    if (flagsignkey) {
        crypto_sign_ed25519_keypair(pk, sk);
        outs("Ed25519 pk: ");
        for (i = 0; i < crypto_sign_ed25519_PUBLICKEYBYTES; ++i) {
            ch = "0123456789abcdef"[15 & (int) (pk[i] >> 4)]; out(&ch, 1);
            ch = "0123456789abcdef"[15 & (int) (pk[i] >> 0)]; out(&ch, 1);
        }
        outs("\n");
        outs("Ed25519 sk: ");
        for (i = 0; i < crypto_sign_ed25519_SECRETKEYBYTES; ++i) {
            ch = "0123456789abcdef"[15 & (int) (sk[i] >> 4)]; out(&ch, 1);
            ch = "0123456789abcdef"[15 & (int) (sk[i] >> 0)]; out(&ch, 1);
        }
        outs("\n");
    }
    flush();

    if (fsyncfd(1) == -1) die_fatal("unable to write output", 0);
    _exit(0);
}
