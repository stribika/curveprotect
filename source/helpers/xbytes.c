/*
20121218
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include "writeall.h"
#include "fsyncfd.h"
#include "strtonum.h"
#include "die.h"
#include "e.h"
#include "str.h"
#include "byte.h"


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

static unsigned char buf[4096];

int main(int argc, char **argv) {

    long long i, l;

    if (!argv[0]) die_usage(0);
    if (!argv[1]) die_usage(0);
    if (str_equal(argv[1], "-h")) die_usage(0);
    if (!strtonum(&l, argv[1]) || l < 0) die_usage("unable to parse number");

    byte_zero(buf, sizeof buf);

    while (l > 0) {
        i = l;
        if (i > sizeof buf) i = sizeof buf;
        xfce(buf, i);
        if (writeall(1, buf, i) == -1) die_fatal("unable to write output", 0);
        l -= i;
    }
    if (fsyncfd(1) == -1) die_fatal("unable to write output", 0);
    _exit(0);
}
