/*
20121219
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include <sys/types.h>
#include "die.h"
#include "e.h"
#include "numtostr.h"
#include "pathexec.h"
#include "str.h"

#define FATAL "effectiveenvuidgid: fatal: "

#define USAGE "\
\n\
effectiveenvuidgid: usage:\n\
\n\
 name:\n\
   effectiveenvuidgid - set effective $UID and $GID\n\
\n\
 syntax:\n\
   effectiveenvuidgid [options] program\n\
\n\
 description:\n\
   effectiveenvuidgid sets effective $UID, $GID and runs program\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 arguments:\n\
   program (mandatory): 'program' consists of one or more arguments\n\
\n\
 example:\n\
   effectiveenvuidgid printenv UID\n\
   effectiveenvuidgid printenv GID\n\
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

    if (!argv[0]) die_usage();
    if (!argv[1]) die_usage();
    if (str_equal(argv[1], "-h")) die_usage();

    x = numtostr(0, geteuid());
    if (!pathexec_env("UID", x)) die_fatal("unable to set environment variable UID", 0); 
    x = numtostr(0, getegid());
    if (!pathexec_env("GID", x)) die_fatal("unable to set environment variable GID", 0); 

    pathexec(argv + 1);
    die_fatal("unable to run", argv[1]);
    return 111;
}
