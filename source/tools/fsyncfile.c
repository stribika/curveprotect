/*
20130529
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include "die.h"
#include "e.h"
#include "str.h"
#include "open.h"

#define FATAL "fsyncfile: fatal: "

#define USAGE "\
\n\
fsyncfile: usage:\n\
\n\
 name:\n\
   fsyncfile - open file and flush data to disk\n\
\n\
 syntax:\n\
   fsyncfile fn\n\
\n\
 description:\n\
   fsyncfile opens file and calls fsync syscall\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 arguments:\n\
   fn    (mandatory): file\n\
\n\
"

void die_usage(const char *s) {
    if (s) die_4(100, USAGE, FATAL, s, "\n");
    die_1(100, USAGE);
}

void die_fatal(const char *trouble, const char *f) {
    if (f) die_7(111, FATAL, trouble, " ", f, ": ", e_str(errno), "\n");
    die_5(111, FATAL, trouble, ": ", e_str(errno), "\n");
}

int main(int argc, char **argv) {

    int fd;

    if (!argv[0]) die_usage(0);
    if (!argv[1]) die_usage("missing fn");
    if (str_equal(argv[1], "-h")) die_usage(0);

    fd = open_write(argv[1]);
    if (fd == -1) die_fatal("unable to open file", argv[1]);
    if (fsync(fd) == -1) die_fatal("unable to fsync file", argv[1]);
    if (close(fd) == -1) die_fatal("unable to fsync file", argv[1]);
    _exit(0);
}
