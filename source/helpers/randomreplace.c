/*
20121218
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include "die.h"
#include "e.h"
#include "readinput.h"
#include "writeall.h"
#include "randombytes.h"
#include "crypto_uint32.h"
#include "str.h"
#include "fsyncfd.h"

#define FATAL "randomreplace: fatal: "


#define USAGE "\
\n\
randomreplace: usage:\n\
\n\
 name:\n\
   randomreplace - replace random byte in byte-stream\n\
\n\
 syntax:\n\
   randomreplace [options] <input >output\n\
\n\
 description:\n\
   randomreplace replaces random byte in byte-stream and\n\
   writes byte-stream output\n\
\n\
 options:\n\
   -h (optional): print this help\n\
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

int main(int argc, char **argv) {
    
    long long mlen;
    unsigned char *m;
    crypto_uint32 a, b;

    if (argv[0])
        if (argv[1])
            if (str_equal(argv[1], "-h"))
                die_usage();


    /* read message */
    if (readinput(0, &m, &mlen) == -1) die_fatal("unable to read input", 0);
    if (mlen == 0) _exit(0);

    randombytes((unsigned char *)&a, 4);
    randombytes((unsigned char *)&b, 4);
    m[a % mlen] += 1 + (b % 255);

    if (writeall(1, m, mlen) == -1) die_fatal("unable to write output", 0);
    if (fsyncfd(1) == -1) die_fatal("unable to write output", 0);
    _exit(0);
}
