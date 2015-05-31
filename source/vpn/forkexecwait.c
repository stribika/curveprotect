#include "forkexecwait.h"
#include <unistd.h>
#include "wait.h"
#include "pathexec.h"


int forkexecwait(char **run){

    int pid;
    int r;
    int status;


    pid = fork();
    if (pid == -1){
        return -1;
        /* die_fork(); */
    }

    if (pid == 0){
        pathexec(run);
        _exit(111);
        /* die_exec(run); */
    }

    r = wait_pid(&status,pid);
    if (r == -1) return -1;

    r = wait_crashed(status);
    if (r){
        /* warn_childcrashed(run, sig_str(r)); */
        return -1;
    }

    r = wait_exitcode(status);
    if (r){
        /* strnum[fmt_ulong(strnum,r)] = 0;
        warn_childfailed(run, strnum); */
        return -1;
    }
    return 0;
}

