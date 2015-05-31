/*
20120524
Jan Mojzis
Public domain.
*/

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "die.h"
#include "e.h"
#include "pathexec.h"
#include "deepsleep.h"
#include "env.h"
#include "strtonum.h"
#include "str.h"

#define FATAL "sigpg: fatal: "

#define USAGE "\
\n\
sigpg: usage:\n\
\n\
 name:\n\
   sigpg - send signal to childs\n\
\n\
 syntax:\n\
   sigpg program [args ...]\n\
\n\
 description:\n\
   sigpg - starts program and after receiving signal, sends the signal to childs\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 arguments:\n\
   program (mandatory): 'program' consists of one or more arguments\n\
\n\
 environment:\n\
   $SIGPG_SLEEP (optional): sleeps '$SIGPG_SLEEP' seconds before sends signal to childs\n\
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


long long pid;
long long sl = 0;

void signalhandler(int s) {

    deepsleep(sl);
    killpg(pid, s);
}

int main(int argc, char **argv, char **envp) {

    int childstatus;

    if (!*argv) die_usage(0);
    if (!*++argv) die_usage("missing program");
    if (str_equal(*argv, "-h")) die_usage(0);

    strtonum(&sl, env_get("SIGPG_SLEEP"));
    if (sl < 0) sl = 0;
    if (sl > 5) sl = 5;

    pid = fork();
    if (pid == -1) die_fatal("unable to fork", 0);

    if (pid == 0) {
        setsid(); 
        setpgid(0, 0);
        signal(SIGTERM, SIG_DFL);
        signal(SIGINT,  SIG_DFL);
        signal(SIGHUP,  SIG_DFL);
        signal(SIGALRM, SIG_DFL);
        pathexec_run(*argv, argv, envp);
        die_fatal("unable to run", argv[0]);
    }

    signal(SIGTERM, signalhandler);
    signal(SIGINT,  signalhandler);
    signal(SIGHUP,  signalhandler);
    signal(SIGALRM, signalhandler);

    while (waitpid(pid, &childstatus, 0) != pid) {};
    _exit(0);
}
