#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <grp.h>
#include "e.h"
#include "extremesandbox.h"

#define EXTREMESANDBOX_BASEUID 141500000
#define EXTREMESANDBOX_MAXPID 1000000000
#define EXTREMESANDBOX_MAXUID EXTREMESANDBOX_BASEUID + EXTREMESANDBOX_MAXPID

#ifndef RLIMIT_NOFILE
#ifdef RLIMIT_OFILE
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif
#endif

/* uids between EXTREMESANDBOX_BASEUID and EXTREMESANDBOX_BASEUID+EXTREMESANDBOX_MAXPID-1
 * must not be used by anything else
 */

static int extremesandbox_killuid(void) {

    long long pid, targetuid;
    int childstatus;

    targetuid = extremesandbox_getuid();
    if (targetuid == -1) return -1;

    switch(pid = fork()) {
        case -1:
            return -1;
        case 0:
            if (setuid(targetuid) == -1) _exit(111);
            kill(-1, SIGKILL);
            _exit(0);
    }
    while (waitpid(pid, &childstatus, 0) != pid) {};
    if (childstatus == 0) return 0;
    if (!WIFEXITED(childstatus) && WTERMSIG(childstatus) == SIGKILL) return 0;
    return -1;
}

long long extremesandbox_getuid(void) {

    long long pid;

    pid = getpid();
    if (pid < 0) return -1;
    if (pid >= EXTREMESANDBOX_MAXPID) {errno = EINVAL; return -1;}
    return EXTREMESANDBOX_BASEUID + pid;
}

int extremesandbox_checkuid(long long targetuid) {

    if (targetuid < EXTREMESANDBOX_BASEUID) { errno = EINVAL; return -1; }
    if (targetuid >= EXTREMESANDBOX_MAXUID) { errno = EINVAL; return -1; }

    return 0;

}

int extremesandbox_droproot(void) {

    uid_t targetuid;
    gid_t targetgid;

    targetuid = extremesandbox_getuid();
    if (targetuid == -1) return -1;
    targetgid = targetuid;

    if (setgroups(1, &targetgid) == -1) return -1;
    if (setgid(targetgid) == -1) return -1;
    if (setuid(targetuid) == -1) return -1;
    if (getgid() != targetgid) return -1;
    if (getuid() != targetuid) return -1;
    return 0;
}


int extremesandbox(int flagnofile, const char *dir) {

    struct rlimit r;

    /* prohibit new files, new sockets, etc. */
#ifdef RLIMIT_NOFILE
    if (flagnofile) {
        if (getrlimit(RLIMIT_NOFILE, &r) == -1) return -1;
        r.rlim_cur = 0;
        r.rlim_max = 0;
        if (setrlimit(RLIMIT_NOFILE, &r) == -1) return -1;
    }
#endif

    if (geteuid() == 0) {

        /* prohibit access to filesystem */
        if (!dir) dir = ".";
        if (chdir(dir) == -1) return -1;
        if (chroot(dir) == -1) return -1;

        /* double-check that uid isn't hanging around */
        if (extremesandbox_killuid() == -1) return -1;

        /* prohibit kill, ptrace, etc. */
        if (extremesandbox_droproot() == -1) return -1;
    }

    /* prohibit fork */
#ifdef RLIMIT_NPROC
    if (getrlimit(RLIMIT_NPROC, &r) == -1) return -1;
    r.rlim_cur = 0;
    r.rlim_max = 0;
    if (setrlimit(RLIMIT_NPROC, &r) == -1) return -1;
#endif
    return 0;
}
