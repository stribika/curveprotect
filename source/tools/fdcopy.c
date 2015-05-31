/*
20120524
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include "die.h"
#include "e.h"
#include "strtonum.h"
#include "numtostr.h"
#include "env.h"
#include "milliseconds.h"
#include "byte.h"
#include "blocking.h"
#include "extremesandbox.h"
#include "str.h"

#define FATAL "fdcopy: fatal: "

void die_fatal(const char *trouble, const char *fn) {

    if (errno) {
        if (fn) die_7(111, FATAL, trouble, " ", fn, ": ", e_str(errno), "\n");
        die_5(111, FATAL, trouble, ": ", e_str(errno), "\n");
    }
    if (fn) die_5(111, FATAL, trouble, " ", fn, "\n");
    die_3(111, FATAL, trouble, "\n");
}


long long gettimeout(char *name) {

    long long t = 0;

    if (t <= 0) strtonum(&t, env_get(name));
    if (t <= 0) strtonum(&t, env_get("TIMEOUT"));
    if (t <= 0) t = 7200;
    if (t > 86400) t = 86400;
    return t;
}

long long getfd(long long xdefault, char *name) {

    long long x = 0;

    if (!strtonum(&x, env_get(name))) {
        return xdefault;
    }
    return x;
}

char buf1[8192];
long long buflen1 = 0;
char buf2[8192];
long long buflen2 = 0;


void die(int signum) { _exit(111); }


int main(int argc, char **argv) {

    int fdok = 1;
    struct pollfd p[4], *x;
    long long len, i, r, now;
    long long din1, dout1, din2, dout2, t, tm;
    long long readtimeout1, readtimeout2;
    long long writetimeout1, writetimeout2;

    long long fdin1;
    long long fdout1;
    long long fdin2;
    long long fdout2;

    char *pkstr;
    long long pkstrlen;

    if (extremesandbox(0, env_get("EMPTYDIRECTORY")) == -1)
        die_fatal("unable to setup extremesandbox", 0);

    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, die);

    pkstr = env_get("REMOTEPK");
    if (pkstr && argv[0] && argv[1]) {
        pkstrlen = str_len(pkstr);
        if (pkstrlen == str_len(argv[1])) {
            byte_copy(argv[1], pkstrlen, pkstr);
        }
    }

    fdin1  = getfd(0, "FDINLOCAL");
    fdout1 = getfd(7, "FDOUTREMOTE");
    fdin2  = getfd(6, "FDINREMOTE");
    fdout2 = getfd(1, "FDOUTLOCAL");

    readtimeout1  = gettimeout("READTIMEOUTLOCAL");
    readtimeout2  = gettimeout("READTIMEOUTREMOTE");
    writetimeout1 = gettimeout("WRITETIMEOUTREMOTE");
    writetimeout2 = gettimeout("WRITETIMEOUTLOCAL");

    blocking_disable(fdin1);
    blocking_disable(fdout1);
    blocking_disable(fdin2);
    blocking_disable(fdout2);

    now = milliseconds();
    din1 = now + readtimeout1 * 1000;
    din2 = now + readtimeout2 * 1000;
    dout1 = 9223372036854775807LL;
    dout2 = 9223372036854775807LL;

    while (fdok || buflen1 > 0 || buflen2 > 0) {

        len = 0;
        x = p;
        t = 9223372036854775807LL;

        if (buflen1 > 0) {
            if (dout1 < t) t = dout1;
            x->fd = fdout1;
            x->events = POLLOUT;
            ++len; ++x;
        }
        if (buflen2 > 0) {
            if (dout2 < t) t = dout2;
            x->fd = fdout2;
            x->events = POLLOUT;
            ++len; ++x;
        }
        if (fdok && buflen1 == 0) {
            if (din1 < t) t = din1;
            x->fd = fdin1;
            x->events = POLLIN;
            ++len; ++x;
        }
        if (fdok && buflen2 == 0) {
            if (din2 < t) t = din2;
            x->fd = fdin2;
            x->events = POLLIN;
            ++len; ++x;
        }

        tm = t - milliseconds();
        if (tm <= 0) tm = 20;
        if (tm > 1000000) tm = 1000000;

        poll(p, len, tm);

        now = milliseconds();

        for (i = 0; i < len; ++i) {
            if (p[i].revents) {
                if (p[i].fd == fdin1) {
                    r = read(fdin1, buf1, sizeof buf1 - buflen1);
                    if (r == -1) {
                        if (errno == EAGAIN) continue;
                        if (errno == EINTR) continue;
                        if (errno == EWOULDBLOCK) continue;
                        die_fatal("unable to read from filedescriptor",  numtostr(0, fdin1));
                    }
                    if (r == 0) { fdok = 0; close(fdin1); close(fdin2); alarm(3600); continue; }
                    buflen1 += r;
                    din1 = now + readtimeout1 * 1000;
                    dout1 = now + writetimeout1 * 1000;
                    continue;
                }

                if (p[i].fd == fdin2) {
                    r = read(fdin2, buf2, sizeof buf2 - buflen2);
                    if (r == -1) {
                        if (errno == EAGAIN) continue;
                        if (errno == EINTR) continue;
                        if (errno == EWOULDBLOCK) continue;
                        die_fatal("unable to read from filedescriptor", numtostr(0, fdin2));
                    }
                    if (r == 0) { fdok = 0; close(fdin1); close(fdin2); alarm(3600); continue; }
                    buflen2 += r;
                    din2 = now + readtimeout2 * 1000;
                    dout2 = now + writetimeout2 * 1000;
                    continue;
                }

                if (p[i].fd == fdout1) {
                    r = write(fdout1, buf1, buflen1);
                    if (r == -1) {
                        if (errno == EAGAIN) continue;
                        if (errno == EINTR) continue;
                        if (errno == EWOULDBLOCK) continue;
                        die_fatal("unable to write to filedescriptor", numtostr(0, fdout1));
                    }
                    byte_copy(buf1, buflen1 - r, buf1 + r);
                    buflen1 -= r;
                    if (!buflen1 && !fdok) close(fdout1);
                    continue;
                }

                if (p[i].fd == fdout2) {
                    r = write(fdout2, buf2, buflen2);
                    if (r == -1) {
                        if (errno == EAGAIN) continue;
                        if (errno == EINTR) continue;
                        if (errno == EWOULDBLOCK) continue;
                        die_fatal("unable to write to filedescriptor", numtostr(0, fdout2));
                    }
                    byte_copy(buf2, buflen2 - r, buf2 + r);
                    buflen2 -= r;
                    if (!buflen2 && !fdok) close(fdout2);
                    continue;
                }
            }
        }
        if ((din1 - now) <= 0) { errno = ETIMEDOUT; die_fatal("unable to read from filedescriptor", numtostr(0, fdin1)); }
        if ((din2 - now) <= 0) { errno = ETIMEDOUT; die_fatal("unable to read from filedescriptor", numtostr(0, fdin2)); }
        if (buflen1 > 0 && (dout1 - now) <= 0) { errno = ETIMEDOUT; die_fatal("unable to write to filedescriptor", numtostr(0, fdout1)); }
        if (buflen2 > 0 && (dout2 - now) <= 0) { errno = ETIMEDOUT; die_fatal("unable to write to filedescriptor", numtostr(0, fdout2)); }
    }

    _exit(0);
}
