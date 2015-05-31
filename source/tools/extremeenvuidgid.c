/*
20120422
Jan Mojzis
Public domain.
*/

#include "die.h"
#include "extremesandbox.h"
#include "e.h"
#include "numtostr.h"
#include "pathexec.h"
#include "str.h"

#define FATAL "extremeenvuidgid: fatal: "


#define USAGE "\
\n\
extremeenvuidgid: usage:\n\
\n\
 name:\n\
   extremeenvuidgid - set unique $UID and $GID\n\
\n\
 syntax:\n\
   extremeenvuidgid [options] program\n\
\n\
 description:\n\
   extremeenvuidgid sets unique $UID, $GID and runs program\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 arguments:\n\
   program (mandatory): 'program' consists of one or more arguments\n\
\n\
 example:\n\
   extremeenvuidgid printenv UID\n\
   extremeenvuidgid printenv GID\n\
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

    long long targetuid;
    char *x;

    if (!argv[0]) die_usage();
    if (!argv[1]) die_usage();
    if (str_equal(argv[1], "-h")) die_usage();

    targetuid = extremesandbox_getuid();
    if (targetuid == -1) die_fatal("unable to extremesandbox_getuid", 0);

    x = numtostr(0, targetuid);

    if (!pathexec_env("UID", x)) die_fatal("unable to set environment variable UID", 0); 
    if (!pathexec_env("GID", x)) die_fatal("unable to set environment variable GID", 0); 

    pathexec(argv + 1);
    die_fatal("unable to run", argv[1]);
    return 111;
}
