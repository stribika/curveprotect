/*
20121229
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include "str.h"
#include "die.h"
#include "hexparse.h"
#include "env.h"
#include "providerparse.h"
#include "crypto_uint32.h"
#include "byte.h"
#include "e.h"
#include "buffer.h"
#include "dnscrypt.h"
#include "stralloc.h"
#include "dns.h"
#include "stralloc.h"
#include "fsyncfd.h"
#include "seconds.h"
#include "strtonum.h"

#define FATAL "dnscryptmakecert: fatal: "
#define WARNING "dnscryptmakecert: warning: "

#define USAGE "\
\n\
dnscryptmakecert: usage:\n\
\n\
 name\n\
   dnscryptmakecert - print DNSCrypt certificate\n\
\n\
 description\n\
   dnscryptmakecert makes signed DNSCrypt certificate using specified $SECRETKEY, serverpk, period and\n\
   prints it in tinydns or bind format for specified provider\n\
\n\
 syntax\n\
   dnscryptmakecert [options] provider serverpk period\n\
\n\
 options:\n\
   -q (optional): no error messages\n\
   -Q (optional): print error messages (default)\n\
   -v (optional): print extra information\n\
   -h (optional): print this help\n\
   -b (optional): bind format\n\
   -t (optional): tinydns format (default)\n\
\n\
 arguments:\n\
   provider (mandatory): DNSCrypt provider name\n\
   serverpk (mandatory): servers DNSCurve public-key\n\
   period   (mandatory): DNSCrypt certificate validity period\n\
\n\
 environment:\n\
   SECRETKEY (mandatory): DNSCrypt provider secter-key\n\
\n\
 example:\n\
   SECRETKEY=ad547da8c53c957ec7e4fad07334915af9fa3d101d0a6b78665ccbf7075a09992d098ab53a532ba35a0bea3da9675300091b5f01fc42a481601deb473d21032e\n\
   export SECRETKEY\n\
   dnscryptmakecert -b dnscrypt-cert.test 4d14c9b91f1a83543d5c585239dc0da9cbe493752bd85644c51cbebc71fac138 31536000\n\
\
"

static int flagverbose = 1;

static void die_usage(const char *s) {
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


static unsigned char serverpk[32];
static unsigned char providersk[64];
static unsigned char outbuf[DNSCRYPT_LEN];
static stralloc providername = {0};


static crypto_uint32 now(void) {
    return (crypto_uint32)seconds();
}

static int periodparse(crypto_uint32 *p, const char *x) {

    long long u;

    if (!x) return 0;
    if (!strtonum(&u, x) || u < 3600 || u > 315360000) return 0;
    *p = u;
    return 1;
}

static void out(const void *xv, long long len) {
    const char *x = xv;
    if (buffer_put(buffer_1, x, len) == -1) die_fatal("unable to write output", 0);
}

static void outs(const char *x) {
    if (buffer_puts(buffer_1, x) == -1) die_fatal("unable to write output", 0);
}

static void flush(void) {
    if (buffer_flush(buffer_1) == -1) die_fatal("unable to write output", 0);
}


static void outescape(unsigned char *x, long long len) {

    long long i;
    unsigned char ch;
    unsigned char buf[4];


    for (i = 0; i < len; ++i) {
        ch = x[i];

        if (((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) || ((ch >= '0') && (ch <= '9')) || (ch == '-') || (ch == '_')) {
            out((char *)&ch, 1);
        }
        else {
            buf[3] = '0' + (ch & 7); ch >>= 3;
            buf[2] = '0' + (ch & 7); ch >>= 3;
            buf[1] = '0' + (ch & 7);
            buf[0] = '\\';
            out((char *)buf, 4);
        }
    }
}

static void outescapeb(unsigned char *x, long long len) {

    long long i;
    unsigned char ch;
    unsigned char buf[4];


    for (i = 0; i < len; ++i) {
        ch = x[i];

        if (((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) || ((ch >= '0') && (ch <= '9')) || (ch == '-') || (ch == '_')) {
            out((char *)&ch, 1);
        }
        else {
            buf[3] = '0' + (ch & 9); ch >>= 3;
            buf[2] = '0' + (ch & 9); ch >>= 3;
            buf[1] = '0' + (ch & 9);
            buf[0] = '\\';
            out((char *)buf, 4);
        }
    }
}

static unsigned char *q = 0;

int main(int argc, char **argv) {

    char *x;
    crypto_uint32 periodsince, periodto, period = 0;
    int flagbind = 0;

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
            if (*x == 'b') { flagbind = 1; continue; }
            if (*x == 't') { flagbind = 0; continue; }
            die_usage(0);
        }
    }
    if (!providerparse(&providername, &q, *++argv)) { 
        if (errno == ENOMEM) die_fatal("out of memory", 0);
        die_usage("provider must be at most 255 bytes, at most 63 bytes between dots");
    }
    if (!hexparse(serverpk, sizeof serverpk, *++argv)) die_usage("serverpk must be exactly 64 hex characters");
    if (!periodparse(&period, *++argv)) die_usage("period must be number in seconds <3600 - 315360000>");

    x = env_get("SECRETKEY");
    if (!x) die_usage("$SECRETKEY not set");
    if (!hexparse(providersk, sizeof providersk, x)) die_usage("$SECRETKEY must be exactly 128 hex characters");
    byte_zero(x, str_len(x));

    periodsince = now();
    periodto = periodsince + period;
    dnscrypt_pack(outbuf, providersk, serverpk, periodsince, periodsince, periodto);

    if (flagbind) {
        out(providername.s, providername.len);
        outs(". 3600 IN TXT \"");
        outescapeb(outbuf, sizeof outbuf);
        outs("\"\n");
    }
    else {
        outs("\'");
        out(providername.s, providername.len);
        outs(":");
        outescape(outbuf, sizeof outbuf);
        outs(":3600\n");
    }
    flush();

    if (fsyncfd(1) == -1) die_fatal("unable to write output", 0);
    _exit(0);
}
