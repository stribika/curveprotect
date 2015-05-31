/*
20120612
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include "fastrandommod.h"
#include "writeall.h"
#include "fsyncfd.h"
#include "strtonum.h"
#include "die.h"
#include "e.h"
#include "str.h"

#define FATAL "randomtext: fatal: "

#define USAGE "\
\n\
randomtext: usage:\n\
\n\
 name:\n\
   randomtext - print random text-stream\n\
\n\
 syntax:\n\
   randomtext [options] length [dictionary]\n\
\n\
 description:\n\
   randomtext prints random text-stream using characters from selected dictionary\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 arguments:\n\
   length (mandatory): text length\n\
   dictionary (optional): default 'abcdefghijklmnopqrstuvwyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'\n\
\n\
 example:\n\
   randomtext 28 'abcdefghijklmnopqrstuvwyz'\n\
   randomtext 23 'abcdefghijklmnopqrstuvwyzABCDEFGHIJKLMNOPQRSTUVWXYZ'\n\
   randomtext 22 'abcdefghijklmnopqrstuvwyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'\n\
   randomtext 21 'abcdefghijklmnopqrstuvwyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.'\n\
   randomtext 20 'abcdefghijklmnopqrstuvwyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789~!@#$%^&*()_-+={}[]|\\:;<>,.?/'\n\
\n\
"


static void die_usage(const char *s) {
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

static char buf[256];
static long long buflen = 0;


static void flush(void) {
    if (writeall(1, buf, buflen) == -1) die_fatal("unable to write output", 0);
    buflen = 0;
}

static void outch(const char ch) {
    if (buflen >= sizeof buf) flush();
    buf[buflen++] = ch;
    return;
}

static void die(void) {
    outch('\n');
    flush();
    if (fsyncfd(1) == -1) die_fatal("unable to write output", 0);
    _exit(0);
}

static const char *defaultdict="abcdefghijklmnopqrstuvwyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

int main(int argc, char **argv) {

    long long i, r, l;
    const char *d;
    long long dlen;

    if (!argv[0]) die_usage(0);
    if (!argv[1]) die_usage(0);
    if (str_equal(argv[1], "-h")) die_usage(0);
    if (!strtonum(&l, argv[1]) || l < 0) die_usage("unable to parse number");

    d = argv[2];
    if (!d || str_len(d) < 1) d = defaultdict;
    dlen = str_len(d);

    for(i = 0; i < l; ++i) {
        r = fastrandommod(dlen);
        outch(d[r]);
    }
    die();
    return 0;
}
