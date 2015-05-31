/*
20120519
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include "die.h"
#include "e.h"
#include "str.h"
#include "warn.h"
#include "env.h"
#include "strtonum.h"
#include "safechown.h"

#define FATAL "uidgidchown: fatal: "
#define WARNING "uidgidchown: warning: "

#define USAGE "\
\n\
uidgidchown: usage:\n\
\n\
 name:\n\
   uidgidchown - change file owner and group\n\
\n\
 syntax:\n\
   uidgidchown [options] file1 file2 ...\n\
\n\
 description:\n\
   uidgidchown changes the user and group ownership of each given file to $UID and $GID\n\
\n\
 options:\n\
   -h (optional): print this help\n\
   -R (optional): operate on directories recursively\n\
\n\
 environment:\n\
   $UID (mandatory): user ID\n\
   $GID (mandatory): group ID\n\
\n\
 example:\n\
   extremeenvuidgid uidgidchown file\n\
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

    long long uid, gid, ret = 0;
    const char *x;
    int flagrecursive = 0;

    if (!*argv) die_usage(0);
    if (!*++argv) die_usage(0);

    if (str_equal(*argv, "-h")) die_usage(0);
    if (str_equal(*argv, "-R")) { flagrecursive = 1; ++argv; }
    if (!*argv) die_usage(0);

    x = env_get("UID");
    if (!x) { errno = 0; die_usage("$UID not set"); }
    if (!strtonum(&uid, x)) die_fatal("invalid $UID:", x);

    x = env_get("GID");
    if (!x) { errno = 0; die_usage("$GID not set"); }
    if (!strtonum(&gid, x)) die_fatal("invalid $GID:", x);

    while(*argv) {
        if (safechown(*argv, uid, gid, flagrecursive) == -1) { ret = 111; warn_6(WARNING, "unable to change ownership of '", *argv, "': ", e_str(errno), "\n"); }
        ++argv;
    }
    _exit(ret);
}
