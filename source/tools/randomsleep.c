/*
20130103
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include <poll.h>
#include "die.h"
#include "e.h"
#include "str.h"
#include "milliseconds.h"
#include "strtonum.h"
#include "fastrandommod.h"

#define FATAL "randomsleep: fatal: "


#define USAGE "\
\n\
randomsleep: usage:\n\
\n\
 name:\n\
   randomsleep - delay for a random amount of time\n\
\n\
 syntax:\n\
   randomsleep [options] number1 [number1]\n\
\n\
 description:\n\
   randomsleep - sleeps randomly from 'number1' seconds to 'number2' seconds\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 arguments:\n\
   number1 (mandatory): lower limit in seconds\n\
   number2 (optional): upper limit in seconds (default 2*number1)\n\
\n\
 example:\n\
   randomsleep 1 \n\
\n\
"

void die_usage(const char *s) {
  if (s) die_4(100, USAGE, FATAL, s, "\n");
  die_1(100, USAGE);
}

void die_fatal(const char *trouble, const char *fn) {

    if (errno) {
        if (fn) die_7(111, FATAL, trouble, " ", fn, ": ", e_str(errno), "\n");
        die_5(111, FATAL, trouble, ": ", e_str(errno), "\n");
    }
    if (fn) die_5(111, FATAL, trouble, " ", fn, "\n");
    die_3(111, FATAL, trouble, "\n");
}


int main(int argc, char **argv) {

    long long l1, l2 = -1, tm, deadline;
    struct pollfd p;

    if (!argv[0]) die_usage(0);
    if (!argv[1]) die_usage(0);
    if (str_equal(argv[1], "-h")) die_usage(0);
    if (!strtonum(&l1, argv[1]) || l1 < 0) die_usage("unable to parse number1");
    if (!argv[2]) l2 = 2 * l1;
    if (l2 == -1) if (!strtonum(&l2, argv[2]) || l2 < 0) die_usage("unable to parse number2");
    if (l2 < l1) die_usage("number2 must be greater or equal than number1");

    deadline = milliseconds() + 1000*(l1 + fastrandommod(l2 - l1 + 1));

    p.fd = 0;
    p.events = POLLIN;
    for (;;) {
        tm = deadline - milliseconds();
        if (tm <= 0) break;
        if (tm > 10000) tm = 10000;
        poll(&p, 0, tm);
    }
    _exit(0);
}
