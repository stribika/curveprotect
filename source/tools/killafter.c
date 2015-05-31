#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "die.h"
#include "e.h"
#include "pathexec.h"
#include "strtonum.h"
#include "str.h"

#define FATAL "killafter: fatal: "

#define USAGE "\
\n\
killafter: usage:\n\
\n\
 name:\n\
   killafter - kill childs after specified amount of time\n\
\n\
 syntax:\n\
   killafter seconds program [args ...]\n\
\n\
 description:\n\
   killafter starts program and after specified amount od time kills it's childs\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 arguments:\n\
   seconds (mandatory): time in seconds\n\
   program (mandatory): 'program' consists of one or more arguments\n\
\n\
 example:\n\
   killafter 1 sleep 2\n\
   killafter 2 sleep 1\n\
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

void timeout(int s) {

    killpg(pid, SIGTERM);
    kill(getpid(), SIGALRM);
    _exit(111);
}


int main(int argc, char **argv, char **envp) {

    long long seconds;
    struct sigaction sa;
    int w;

    if (!*argv) die_usage(0);
    if (!*++argv) die_usage("missing seconds");
    if (str_equal(*argv, "-h")) die_usage(0);

    if (!strtonum(&seconds, *argv) || seconds < 0) {
        errno = EINVAL;
        die_fatal("unable to parse number", argv[1]);
    }

    if (!*++argv) die_usage("missing program");

    pid = fork();
    if (pid == -1) die_fatal("unable to fork", 0);

    if (pid == 0) {
        setpgid(0, 0);
        pathexec_run(*argv, argv, envp);
        die_fatal("unable to run", argv[0]);
    }

    sa.sa_handler = timeout;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND | SA_NODEFER;
    sigaction(SIGALRM, &sa, 0);
    if (seconds == 0) seconds = 1; /* XXX */
    alarm(seconds);

    while (wait(&w) != pid) ;
    if (WIFEXITED(w)) _exit(WEXITSTATUS(w));
    _exit(111);
}

