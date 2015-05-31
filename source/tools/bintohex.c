/*
20121218
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include "writeall.h"
#include "die.h"
#include "e.h"
#include "str.h"
#include "fsyncfd.h"

#define FATAL "bintohex: fatal: "


#define USAGE "\
\n\
bintohex: usage:\n\
\n\
 name:\n\
   bintohex - convert binary stream to hexadecimal stream\n\
\n\
 syntax:\n\
   bintohex [options] <input >output\n\
\n\
 description:\n\
   bintohex converts binary stream to hexadecimal stream\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 example:\n\
   randomtext 10 0123456789abcdef | hextobin | bintohex\n\
\n\
"

void die_usage(void) { die_1(100, USAGE); }

void die_fatal(const char *trouble, const char *fn) {

    if (errno) {
        if (fn) die_7(111, FATAL, trouble, " ", fn, ": ", e_str(errno), "\n");
        die_5(111, FATAL, trouble, ": ", e_str(errno), "\n");
    }
    if (fn) die_5(111, FATAL, trouble, " ", fn, "\n");
    die_3(111, FATAL, trouble, "\n");
}

char buf[256];
long long buflen = 0;


static void flush(void) {
    if (writeall(1, buf, buflen) == -1) die_fatal("unable to write output", 0);
    buflen = 0;
}

static void die(void) {

    flush();
    if (fsyncfd(1) == -1) die_fatal("unable to write output", 0);
    _exit(0);
}

static void outch(const char ch) {
    if (buflen >= sizeof buf) flush();
    buf[buflen++] = ch;
    return;
}

char inbuf[128];

int main(int argc, char **argv) {

    long long r, i;

    if (argv[0])
        if (argv[1])
            if (str_equal(argv[1], "-h"))
                die_usage();

    for (;;) {
        r = read(0, inbuf, sizeof inbuf);
        if (r == -1) die_fatal("unable to read input", 0);
        if (r == 0) break;

        for (i = 0; i < r; ++i) {
            outch("0123456789abcdef"[15 & (inbuf[i] >> 4)]);
            outch("0123456789abcdef"[15 & inbuf[i]]);
        }
    }
    outch('\n');
    die();
    return 0;
}
