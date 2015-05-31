/*
20120329
Jan Mojzis
Public domain.
*/


#include <unistd.h>
#include "fastrandommod.h"
#include "writeall.h"
#include "fsyncfd.h"
#include "die.h"
#include "e.h"
#include "str.h"

#define FATAL "shuffleargs: fatal: "

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

static void outs(const char *x) {

    long long i;

    for(i = 0; x[i]; ++i) {
        if (buflen >= sizeof buf) flush();
        buf[buflen++] = x[i];
    }
}

int main(int argc, char **argv) {

    long long i,j;
    char *x;

    if (argc < 2) _exit(0);
    --argc;
    ++argv;

    for(i = 0; i < argc; ++i) {
        j = fastrandommod(argc);
        x = argv[i]; argv[i] = argv[j]; argv[j] = x;
    }

    for (i = 0;i < argc; ++i) {
        outs(argv[i]); outs("\n");
    }
    die();
    return 0;
}
