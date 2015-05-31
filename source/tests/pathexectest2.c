/*
20130606
Jan Mojzis
Public domain.
*/

#include <stdlib.h>
#include "pathexec.h"
#include "env.h"

#define SCRIPT "\
err1='pathexectest2: failed: pathexec_env does not set variable'\n\
err2='pathexectest2: failed: pathexec_env does not clean default environment'\n\
[ x\"$AAA\" = xok ] || ( echo $err1; exit 111; ) \n\
[ x\"$BBB\" = x ]   || ( echo $err2; exit 111; ) \n\
[ x\"$CCC\" = xok ] || ( echo $err1; exit 111; ) \n\
exit 0 \n\
"

int main(int argc, char **argv) {

    char *run[4];
    char *e[1];

    e[0] = 0;


    if (!pathexec_env("AAA", "ok")) return 111;

    if (setenv("BBB", "failed", 1) == -1) return -1;

    if (!pathexec_env("CCC", "failed")) return 111;
    if (!pathexec_env("CCC", "ok")) return 111;

    run[0] = "sh";
    run[1] = "-ec";
    run[2] = SCRIPT;
    run[3] = 0;

    environ = e;
    pathexec(run);
    return 111;
}

