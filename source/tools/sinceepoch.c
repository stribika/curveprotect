/*
20121228
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include "die.h"
#include "e.h"
#include "str.h"
#include "seconds.h"
#include "numtostr.h"
#include "writeall.h"
#include "fsyncfd.h"

#define FATAL "sinceepoch: fatal: "

#define USAGE "\
\n\
sinceepoch: usage:\n\
\n\
 name:\n\
   sinceepoch - print seconds since epoch\n\
\n\
 syntax:\n\
   sinceepoch [options]\n\
\n\
 description:\n\
   sinceepoch - prints number of seconds since epoch\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 example:\n\
   sinceepoch\n\
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

    char *x;
    long long xlen;

    if (argv[0])
        if (argv[1])
            if (str_equal(argv[1], "-h"))
                die_usage();

    x = numtostr(0, seconds());
    xlen = str_len(x);
    x[xlen] = '\n';

    if (writeall(1, x, xlen + 1) == -1) die_fatal("unable to write output", 0);
    if (fsyncfd(1) == -1) die_fatal("unable to write output", 0);
    _exit(0);
}
