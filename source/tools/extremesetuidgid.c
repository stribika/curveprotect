/*
20120422
Jan Mojzis
Public domain.
*/

#include "die.h"
#include "extremesandbox.h"
#include "e.h"
#include "pathexec.h"
#include "str.h"

#define FATAL "extremesetuidgid: fatal: "


#define USAGE "\
\n\
extremesetuidgid: usage:\n\
\n\
 name:\n\
   extremesetuidgid - run program under unique $UID and $GID\n\
\n\
 syntax:\n\
   extremesetuidgid [options] program\n\
\n\
 description:\n\
   extremesetuidgid runs program under unique $UID and $GID\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 arguments:\n\
   program (mandatory): 'program' consists of one or more arguments\n\
\n\
 example:\n\
   extremesetuidgid true\n\
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

int main(int argc, char **argv, char **envp) {

    if (!argv[0]) die_usage();
    if (!argv[1]) die_usage();
    if (str_equal(argv[1], "-h")) die_usage();

    if (extremesandbox_droproot() == -1) die_fatal("unable to drop root privileges", 0);

    pathexec_run(argv[1], argv + 1, envp);
    die_fatal("unable to run", argv[1]);
    return 111;
}
