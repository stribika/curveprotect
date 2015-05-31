/*
20120619
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include "die.h"
#include "e.h"
#include "str.h"
#include "base32encode.h"
#include "readblock.h"
#include "writeall.h"
#include "fsyncfd.h"

#define FATAL "bintobase32: fatal: "


#define USAGE "\
\n\
bintobase32: usage:\n\
\n\
 name:\n\
   bintobase32 - convert binary stream to base32 encoded stream\n\
\n\
 syntax:\n\
   bintobase32 [options] <input >output\n\
\n\
 description:\n\
   bintobase32 converts binary stream to base32 encoded stream\n\
   using dictionary 0123456789bcdfghjklmnpqrstuvwxyz\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 example:\n\
   echo ahoj | bintobase32\n\
\n\
"

static void die_usage(void) { die_1(100, USAGE); }


static void die_fatal(const char *trouble, const char *fn) {

    if (errno) {
        if (fn) die_7(111, FATAL, trouble, " ", fn, ": ", e_str(errno), "\n");
        die_5(111, FATAL, trouble, ": ", e_str(errno), "\n");
    }
    if (fn) die_5(111, FATAL, trouble, " ", fn, "\n");
    die_3(111, FATAL, trouble, "\n");
}


static unsigned char bufr[400];
static unsigned char bufw[641];

static char buf[512];
static long long buflen = 0;

static void flush(void) {
    if (writeall(1, buf, buflen) == -1) die_fatal("unable to write output", 0);
    buflen = 0;
}


static void out(const unsigned char *x, long long len) {

    long long i;

    for(i = 0; i < len; ++i) {
        if (buflen >= sizeof buf) flush();
        buf[buflen++] = x[i];
    }
}

static void die(void) {

    out((unsigned char *)"\n", 1);
    flush();
    if (fsyncfd(1) == -1) die_fatal("unable to write output", 0);
    _exit(0);
}

int main(int argc, char **argv) {

    long long r, w;

    if (argv[0])
        if (argv[1])
            if (str_equal(argv[1], "-h"))
                die_usage();

    for (;;) {
        r = readblock(0, bufr, sizeof bufr);
        if (r == -1) die_fatal("unable to read input", 0);
        if (r == 0) die();

        w = (((r * 8) + 4) / 5);
        if (!base32encode(bufw, w + 1, bufr, r)) { errno = 0; die_fatal("unable to encode to base32", 0); }
        out(bufw, w);
        if (r != sizeof bufr) die();
    }
    return 0;
}
