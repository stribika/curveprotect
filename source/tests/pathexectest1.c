/*
20130606
Jan Mojzis
Public domain.
*/

#include <stdlib.h>
#include "pathexec.h"

#define SCRIPT "\
err='pathexectest1: failed: pathexec_env does not set variable'\n\
[ x\"$AAA\" = xok ] || ( echo $err; exit 111; ) \n\
[ x\"$BBB\" = xok ] || ( echo $err; exit 111; ) \n\
[ x\"$CCC\" = xok ] || ( echo $err; exit 111; ) \n\
[ x\"$DDD\" = xok ] || ( echo $err; exit 111; ) \n\
[ x\"$EEE\" = xok ] || ( echo $err; exit 111; ) \n\
exit 0 \n\
"

int main(int argc, char **argv) {

    char *run[4];

    if (!pathexec_env("AAA", "ok")) return 111;

    if (setenv("BBB", "ok", 1) == -1) return -1;

    if (setenv("CCC", "failed", 1) == -1) return -1;
    if (!pathexec_env("CCC", "ok")) return 111;

    if (!pathexec_env("DDD", "ok")) return 111;
    if (setenv("DDD", "failed", 1) == -1) return -1;

    if (!pathexec_env("EEE", "failed1")) return 111;
    if (!pathexec_env("EEE", "failed2")) return 111;
    if (!pathexec_env("EEE", "failed3")) return 111;
    if (!pathexec_env("EEE", "ok")) return 111;

    run[0] = "sh";
    run[1] = "-ec";
    run[2] = SCRIPT;
    run[3] = 0;
    pathexec(run);
    return 111;
}
