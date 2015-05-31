/*
 * 20120615
 * Jan Mojzis
 * Public domain.
 */

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "timeoutread.h"
#include "die.h"
#include "e.h"
#include "env.h"
#include "pathexec.h"
#include "numtostr.h"
#include "writeall.h"

#define FATAL "jabberproxy: fatal: "


void die_fatal(const char *trouble, const char *fn) {

    if (errno) {
        if (fn) die_7(111, FATAL, trouble, " ", fn, ": ", e_str(errno), "\n");
        die_5(111, FATAL, trouble, ": ", e_str(errno), "\n");
    }
    if (fn) die_5(111, FATAL, trouble, " ", fn, "\n");
    die_3(111, FATAL, trouble, "\n");
}


char buf[1024];
long long buflen = 0;

char host[256];
long long hostlen = 0;

void parsehostname(void) {

    long long i = 0;
    char ch;
    int flagstreamcatch = 0;
    int flagstreamstart = 0;
    int flagstart = 0;
    int flagcatch = 0;
    int pos = 0;

    buf[buflen] = 0;

    for (;;) {
        ch = buf[i++];
        if (!ch)  goto finish;

        if (ch == '\r') continue;
        if (ch == '\n') continue;
        if (ch == '<'){
            pos = 0; flagstart = 0; flagcatch = 0; flagstreamcatch = 0; flagstreamstart = 1;
            for (;;) {
                ch = buf[i++];
                if (!ch)  goto finish;
                if (ch == '>') break;
                if (ch == ' ') if (flagstreamcatch) {pos = 0; flagstart = 1; flagcatch = 0; continue;}

                if (flagstreamstart) {
                    if (ch != "stream:stream"[pos]) if (ch != "STREAM:STREAM"[pos]) {flagstreamstart = 0; continue;}
                    ++pos;
                    if (pos == 13){
                        flagstreamcatch = 1;
                        flagstreamstart = 0;
                        pos = 0;
                    }
                }

                if (!flagstreamcatch) continue;

                if (flagstart) {
                    if (ch != "to="[pos]) if (ch != "TO="[pos]) {flagstart = 0; continue;}
                    ++pos;
                    if (pos == 3){
                        flagcatch = 1;
                        flagstart = 0;
                        pos = 0;
                    }
                    continue;
                }

                if (flagcatch) {
                    if (ch == '"') continue;
                    if (ch == '\'') continue;
                    if (hostlen == sizeof host) {errno = ENOMEM; die_fatal("unable to parse hostname", 0);}
                    host[hostlen++] = ch;
                }
            }
        }
    }

finish:
    if (!hostlen) return;
    if (hostlen == sizeof host) {errno = ENOMEM; die_fatal("unable to parse hostname", 0);}
    host[hostlen++] = 0;
    return;
}



int main(int argc, char **argv, char **envp) {

    long long i, r, child;
    char *run[11];
    char *keydir;
    int tochild[2] = {-1,-1};
    int fromchild[2] = {-1,-1};

    signal(SIGPIPE, SIG_IGN);

    for (;;) {
        if (buflen == sizeof buf) {errno = ENOMEM; die_fatal("unable to read input", 0);}
        r = timeoutread(60, 0, buf + buflen, sizeof buf - buflen);
        if (r <= 0) die_fatal("unable to read input", 0);
        buflen += r;
        parsehostname();
        if (hostlen) break;
    }
    if (!hostlen) {die_fatal("unable to parse hostname", 0);}

    if (pipe(tochild) == -1) die_fatal("unable to create pipe", 0);
    if (pipe(fromchild) == -1) die_fatal("unable to create pipe", 0);

    i = 0;
    child = fork();
    if (child == -1) die_fatal("unable to fork", 0);
    if (child == 0) {
        close(0);
        if (dup(tochild[0]) != 0) die_fatal("unable to dup", 0);
        close(1);
        if (dup(fromchild[1]) != 1) die_fatal("unable to dup", 0);
        close(tochild[1]);
        close(fromchild[0]);

        keydir = env_get("KEYDIR");
        run[i++] = "netclient";
        run[i++] = "-vvUCsj";
        if (keydir) {
            run[i++] = "-k";
            run[i++] = keydir;
        }
        run[i++] = host;
        run[i++] = "5222";
        run[i++] = "fdcopy";
        run[i++] = 0;
        signal(SIGPIPE, SIG_DFL);
        pathexec_run(*run, run, envp);
        die_fatal("unable to run", run[0]);
    }
    close(fromchild[1]);
    close(tochild[0]);

    if (writeall(tochild[1], buf, buflen) == -1) die_fatal("unable to write data", 0);

    if (!pathexec_env("FDINLOCAL", numtostr(0, fromchild[0]))) die_fatal("unable to set environment varible $FDINLOCAL", 0);
    if (!pathexec_env("FDOUTREMOTE", numtostr(0, 1))) die_fatal("unable to set environment varible $FDOUTREMOTE", 0);
    if (!pathexec_env("FDINREMOTE", numtostr(0, 0))) die_fatal("unable to set environment varible $FDINREMOTE", 0);
    if (!pathexec_env("FDOUTLOCAL", numtostr(0, tochild[1]))) die_fatal("unable to set environment varible $FDOUTLOCAL", 0);
    run[0] = "fdcopy";
    run[1] = 0;
    signal(SIGPIPE, SIG_DFL);
    pathexec(run);
    die_fatal("unable to run", run[0]);
    return 111;
}
