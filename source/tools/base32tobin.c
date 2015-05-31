/*
20120619
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include "die.h"
#include "e.h"
#include "str.h"
#include "base32decode.h"
#include "readblock.h"
#include "writeall.h"
#include "fsyncfd.h"

#define FATAL "base32tobin: fatal: "


#define USAGE "\
\n\
base32tobin: usage:\n\
\n\
 name:\n\
   base32tobin - convert base32 encoded stream to binary stream\n\
\n\
 syntax:\n\
   base32tobin [options] <input >output\n\
\n\
 description:\n\
   base32tobin converts base32 encoded stream to binary stream\n\
   using dictionary 0123456789bcdfghjklmnpqrstuvwxyz\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 example:\n\
   echo 13uy6p91 | base32tobin\n\
   echo 13UY6P91 | base32tobin\n\
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


static unsigned char buf[400];

int main(int argc, char **argv) {

    long long r, w;

    if (argv[0])
        if (argv[1])
            if (str_equal(argv[1], "-h"))
                die_usage();

    for (;;) {
        r = readblock(0, buf, sizeof buf);
        if (r == -1) die_fatal("unable to read input", 0);

        for (;;) {
            if (r <= 0) goto end;
            if (buf[r - 1] == '\n') { --r; continue; }
            if (buf[r - 1] == '\r') { --r; continue; }
            break;
        }

        w = (r * 5) / 8;
        if (!base32decode(buf, w + 1, buf, r)) { errno = 0; die_fatal("unable to decode from base32", 0); }
        if (writeall(1, buf, w) == -1) die_fatal("unable to write output", 0);
        if (r != sizeof buf) break;
    }

end:
    if (fsyncfd(1) == -1) die_fatal("unable to write output", 0);
    _exit(0);
}
