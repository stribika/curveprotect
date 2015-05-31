/*
20121218
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include "die.h"
#include "e.h"
#include "readblock.h"
#include "writeall.h"
#include "fsyncfd.h"
#include "str.h"
#include "hexparse.h"

#define FATAL "hextobin: fatal: "

#define USAGE "\
\n\
hextobin: usage:\n\
\n\
 name:\n\
   hextobin - convert hexadecimal stream to binary stream\n\
\n\
 syntax:\n\
   hextobin [options] <input >output\n\
\n\
 description:\n\
   hextobin converts hexadecimal stream to binary stream\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 example:\n\
   randomtext 10 0123456789abcdef | hextobin | bintohex\n\
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

#define BLOCK 4096
static unsigned char buf[BLOCK + 1];

int main(int argc, char **argv) {

    long long r,w;

    if (argv[0])
        if (argv[1])
            if (str_equal(argv[1], "-h"))
                die_usage();

    for (;;) {
        r = readblock(0, buf, BLOCK);
        if (r == -1) die_fatal("unable to read input", 0);

        for (;;) {
            if (r <= 0) goto end;
            if (buf[r - 1] == '\n') { --r; continue; }
            if (buf[r - 1] == '\r') { --r; continue; }
            break;
        }
        buf[r] = 0;
        if (r % 2) { errno = 0; die_fatal("unable to decode from hex", 0); }
        w = r / 2;
        if (!hexparse(buf, w, (char *)buf)) { errno = 0; die_fatal("unable to decode from hex", 0); }
        if (writeall(1, buf, w) == -1) die_fatal("unable to write output", 0);
        if (r != BLOCK) break;
    }

end:
    if (fsyncfd(1) == -1) die_fatal("unable to write output", 0);
    _exit(0);
}
