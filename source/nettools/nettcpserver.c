/*
 * 20130510
 * Jan Mojzis
 * Public domain.
 */

#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include <signal.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "e.h"
#include "portparse.h"
#include "xsocket.h"
#include "strtoip.h"
#include "die.h"
#include "debug.h"
#include "warn.h"
#include "pathexec.h"
#include "strtonum.h"
#include "iptostr.h"
#include "porttostr.h"
#include "blocking.h"
#include "env.h"
#include "numtostr.h"
#include "str.h"

#define FATAL "nettcpserver: fatal: "
#define DEBUG "nettcpserver: debug: "
#define DROP "nettcpserver: warning: dropping connection, "

#define USAGE "\
\n\
nettcpserver: usage:\n\
\n\
 name:\n\
   nettcpserver - accepts incoming TCP connections\n\
\n\
 syntax:\n\
   nettcpserver [options] ip port prog\n\
\n\
 description:\n\
   Nettcpserver waits for connections from TCP clients. For each connection, it runs prog,\n\
   with descriptor 0 reading from the network and descriptor 1 writing to the network.\n\
   Nettcpserver is similar to tcpserver (http://cr.yp.to/ucspi-tcp/tcpserver.html) and\n\
   has this differences:\n\
   - has IPv6 support\n\
   - doesn't gather remote information from DNS or from Authentication Protocol\n\
   - doesn't use rules database\n\
\n\
 options:\n\
   -q (optional): no error messages\n\
   -Q (optional): print error messages (default)\n\
   -v (optional): print extra information\n\
   -c n (optional): allow at most n clients at once (default 100)\n\
   -g gid (optional): switch group ID to gid\n\
   -u uid (optional): switch user ID to uid\n\
   -U (optional): same as -g $GID -u $UID\n\
   -b n (optional): allow a backlog of approximately n TCP SYNs (default 10)\n\
   -I iface (optional): bind to the network interface iface, IPv6 only\n\
   ip: server's IP address\n\
   port: server's TCP port\n\
   prog: run this program\n\
\n\
"

int flagverbose = 1;

void die_usage(const char *s) {
    if (s) die_4(100, USAGE, FATAL, s, "\n");
    die_1(100, USAGE);
}

static void die_fatal(const char *trouble, const char *fn) {

    if (errno) {
        if (fn) die_7(errno, FATAL, trouble, " ", fn, ": ", e_str(errno), "\n");
        die_5(errno, FATAL, trouble, ": ", e_str(errno), "\n");
    }
    if (fn) die_5(111, FATAL, trouble, " ", fn, "\n");
    die_3(111, FATAL, trouble, "\n");
}

static void die_drop(const char *trouble, const char *fn) {

    if (errno) {
        if (fn) die_7(errno, DROP, trouble, " ", fn, ": ", e_str(errno), "\n");
        die_5(errno, DROP, trouble, ": ", e_str(errno), "\n");
    }
    if (fn) die_5(111, DROP, trouble, " ", fn, "\n");
    die_3(111, DROP, trouble, "\n");
}

static void die_nomem(void) {
    die_3(errno, FATAL, e_str(errno), "\n");
}

long long tcpfd;
int tcpfdtype;
unsigned char localip[16] = {0};
unsigned char localport[2] = {0};
long long  localscope;
unsigned char remoteip[16];
unsigned char remoteport[2];
long long  remotescope;

long long backlog = 10;
long long maxclients = 50;
long long numclients = 0;
uid_t uid = 0;
gid_t gid = 0;

void printstatus(void) {

    char strnum1[NUMTOSTR_LEN];
    char strnum2[NUMTOSTR_LEN];

    if (flagverbose < 2) return;

    warn_5("nettcpserver: status: ", numtostr(strnum1, numclients), "/", numtostr(strnum2, maxclients), "\n");
}

void printmaxreached(void) {

    char strnum1[NUMTOSTR_LEN];
    char strnum2[NUMTOSTR_LEN];

    if (flagverbose < 1) return;
    warn_5("nettcpserver: warning: max clients reached: ", numtostr(strnum1, numclients), "/", numtostr(strnum2, maxclients), "\n");
}

void warnignored(const char *x) {
    warn_3("nettcpserver: warning: option ", x, " ignored\n");
}


void sigterm(int x) { die_0(0); }

void sigchld(int x) {

    int childstatus;
    pid_t pid;
    char strnum1[NUMTOSTR_LEN];
    char strnum2[NUMTOSTR_LEN];

    while ((pid = waitpid(-1, &childstatus, WNOHANG)) > 0) {
        if (flagverbose >= 2) {
            warn_5("nettcpserver: end ", numtostr(strnum1, pid), " status ", numtostr(strnum2, childstatus), "\n");
        }
        if (numclients > 0) --numclients; printstatus();
    }
}

long long numparse(const char *x) {

    long long ll;

    if (strtonum(&ll, x)) return ll;
    die_5(100, USAGE, FATAL, "unable to parse number ", x, "\n");
    return -1;
}

void doit(int t, int type) {

    if (flagverbose >= 2) {
        warn_5("nettcpserver: pid ", numtostr(0, getpid()), " from ", iptostr(0, remoteip), "\n");
    }

    xsocket_ipoptionskill(t);
    xsocket_tcpnodelay(t);

    if (xsocket_local(t, type, localip, localport, &localscope) == -1) die_fatal("unable to get local address", 0);

    if (!pathexec_env("PROTO", "TCP")) die_nomem();
    if (!pathexec_env("REMOTEPORT", porttostr(0, remoteport))) die_nomem();
    if (!pathexec_env("REMOTEIP", iptostr(0, remoteip))) die_nomem();
    if (!pathexec_env("LOCALPORT", porttostr(0, localport))) die_nomem();
    if (!pathexec_env("LOCALIP", iptostr(0, localip))) die_nomem();

    if (!pathexec_env("TCPREMOTEPORT", porttostr(0, remoteport))) die_nomem();
    if (!pathexec_env("TCPREMOTEIP", iptostr(0, remoteip))) die_nomem();
    if (!pathexec_env("TCPLOCALPORT", porttostr(0, localport))) die_nomem();
    if (!pathexec_env("TCPLOCALIP", iptostr(0, localip))) die_nomem();
}

int main(int argc, char **argv) {

    char *x, *y;
    int t;
    struct pollfd p[2];
    sigset_t ss;

    sigemptyset(&ss);
    sigaddset(&ss, SIGCHLD);

    if (!argv[0]) die_usage(0);
    for (;;) {
        if (!argv[1]) break;
        if (argv[1][0] != '-') break;
        x = *++argv;
        if (x[0] == '-' && x[1] == 0) break;
        if (x[0] == '-' && x[1] == '-' && x[2] == 0) break;
        while (*++x) {
            if (*x == 'q') { flagverbose = 0; continue; }
            if (*x == 'Q') { flagverbose = 1; continue; }
            if (*x == 'v') { if (flagverbose >= 2) flagverbose = 3; else flagverbose = 2; continue; }
            if (*x == 'c') {
                if (x[1]) { maxclients = numparse(x + 1); break; }
                else if (argv[1]) { maxclients = numparse(*++argv); break; }
            }
            if (*x == 'u') {
                if (x[1]) { uid = numparse(x + 1); break; }
                else if (argv[1]) { uid = numparse(*++argv); break; }
            }
            if (*x == 'g') {
                if (x[1]) { gid = numparse(x + 1); break; }
                else if (argv[1]) { gid = numparse(*++argv); break; }
            }
            if (*x == 'U') {
                y = env_get("UID"); if (!y) die_usage("$UID not set"); uid = numparse(y);
                y = env_get("GID"); if (!y) die_usage("$GID not set"); gid = numparse(y);
                continue;
            }
            if (*x == 'b') {
                if (x[1]) { backlog = numparse(x + 1); break; }
                else if (argv[1]) { backlog = numparse(*++argv); break; }
            }
            if (*x == 'I') {
                if (x[1]) { localscope = xsocket_getscopeid(x + 1); break; }
                if (argv[1]) { localscope = xsocket_getscopeid(*++argv); break; }
            }

            /* not implemented */
            if (*x == '1') { warnignored("-1"); continue; }
            if (*x == '4') { warnignored("-4"); continue; }
            if (*x == '6') { warnignored("-6"); continue; }
            if (*x == 'O') { warnignored("-O"); continue; }
            if (*x == 'o') { warnignored("-o"); continue; }
            if (*x == 'D') { warnignored("-D"); continue; }
            if (*x == 'd') { warnignored("-d"); continue; }
            if (*x == 'H') { warnignored("-H"); continue; }
            if (*x == 'h') { warnignored("-h"); continue; }
            if (*x == 'P') { warnignored("-P"); continue; }
            if (*x == 'p') { warnignored("-p"); continue; }
            if (*x == 'R') { warnignored("-R"); continue; }
            if (*x == 'r') { warnignored("-r"); continue; }
            if (*x == 'X') { warnignored("-X"); continue; }
            if (*x == 'B') {
                warnignored("-B");
                if (x[1]) { break; }
                if (argv[1]) {++argv; break; }
            }
            if (*x == 'x') {
                warnignored("-x");
                if (x[1]) { break; }
                if (argv[1]) {++argv; break; }
            }
            if (*x == 't') {
                warnignored("-t");
                if (x[1]) { break; }
                if (argv[1]) {++argv; break; }
            }
            if (*x == 'l') {
                warnignored("-l");
                if (x[1]) { break; }
                if (argv[1]) {++argv; break; }
            }

            die_usage(0);
        }
    }

    if (backlog < 1 || backlog > 65535) die_usage("backlog must be an integer between 1 and 65535");
    if (maxclients < 1 || maxclients > 65535) die_usage("maxclients must be an integer between 1 and 65535");

    if (!*++argv) die_usage("missing ip");
    if (!strtoip(localip, *argv) && !str_equal(*argv, "") &&  !str_equal(*argv, "0")) die_usage("ip must be an valid IP address");
    if (!*++argv) die_usage("missing port");
    if (!portparse(localport, *argv)) die_usage("port must be an integer between 0 and 65535");
    if (!*++argv) die_usage("missing prog");
    sigprocmask(SIG_BLOCK, &ss, (sigset_t *)0);
    signal(SIGCHLD, sigchld);
    signal(SIGTERM, sigterm);
    signal(SIGPIPE, SIG_IGN);

    /* create server xsocket */
    tcpfdtype= xsocket_type(localip);
    tcpfd = xsocket_tcp(tcpfdtype);
    if (tcpfd == -1) die_fatal("unable to create TCP socket", 0);
    debug_2(flagverbose, DEBUG, "TCP socket: created\n");
    if (xsocket_bind_reuse(tcpfd, tcpfdtype, localip, localport, localscope) == -1) die_fatal("unable to bind socket", 0);
    debug_6(flagverbose, DEBUG, "TCP socket: bind to ", iptostr(0, localip), " ", porttostr(0, localport), "\n");
    if (xsocket_local(tcpfd, tcpfdtype, localip, localport, &localscope) == -1) die_fatal("unable to get local address", 0);
    debug_6(flagverbose, DEBUG, "TCP socket: local address: ", iptostr(0, localip), " ", porttostr(0, localport), "\n");
    if (xsocket_listen(tcpfd, backlog) == -1) die_fatal("unable to listen on TCP socket", 0);
    debug_4(flagverbose, DEBUG, "TCP socket: listen with backlog ", numtostr(0, backlog), "\n");
    blocking_enable(tcpfd);

    /* drop root */
    if (gid) {
        if (setgroups(1, &gid) == -1) die_fatal("unable to set gid", 0);
        if (setgid(gid) == -1)  die_fatal("unable to set gid", 0);
        if (getgid() != gid)  die_fatal("unable to set gid", 0);
        debug_4(flagverbose, DEBUG, "gid: ", numtostr(0, gid), "\n");
    }
    if (uid) {
        if (setuid(uid) == -1)  die_fatal("unable to set uid", 0);
        if (getuid() != uid) die_fatal("unable to set uid", 0);
        debug_4(flagverbose, DEBUG, "uid: ", numtostr(0, gid), "\n");
    }

    /* close stdin, stdout */
    close(0);
    close(1);
    printstatus();

    /* main loop */
    for (;;) {
    
        sigprocmask(SIG_UNBLOCK, &ss, (sigset_t *)0);
        while (numclients >= maxclients) {
            printmaxreached();
            poll(p, 0, -1);
        }
        t = xsocket_accept(tcpfd, tcpfdtype, remoteip, remoteport, &remotescope);
        sigprocmask(SIG_BLOCK, &ss, (sigset_t *)0);

        if (t == -1) continue;
        ++numclients; printstatus();

        switch(fork()) {
            case 0:
                close(tcpfd);
                doit(t, tcpfdtype);
                if (dup2(t, 0) == -1) die_fatal("unable to dup", 0);
                if (dup2(t, 1) == -1) die_fatal("unable to dup", 0);
                if (t > 1) close(t);
                signal(SIGCHLD, SIG_DFL);
                sigprocmask(SIG_UNBLOCK, &ss, (sigset_t *)0);
                signal(SIGPIPE, SIG_DFL);
                signal(SIGTERM, SIG_DFL);
                debug_exec(flagverbose, DEBUG, "executing: ", argv);
                pathexec(argv);
                die_drop("unable to run", *argv);
            case -1:
                die_drop("unable to fork", 0);
                --numclients; printstatus();
        }
        close(t);
    }
}
