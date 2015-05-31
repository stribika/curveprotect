#include "pathexec.h"
#include "die.h"
#include "e.h"

int main(int argc, char **argv, char **envp) {

    if (!argv[0]) die_0(100);
    argv[0] = "curvecpmessage";

    pathexec_run(*argv, argv, envp);
    die_5(111, "netcurvecpmessage: fatal: unable to run ", *argv, ": ", e_str(errno), "\n");
    return 111;
}
