/*
 * 20130507
 * Jan Mojzis
 * Public domain.
 */

#include <signal.h>
#include <poll.h>
#include <unistd.h>
#include "fastrandommod.h"
#include "e.h"
#include "portparse.h"
#include "strtomultiip.h"
#include "strtoip.h"
#include "open.h"
#include "xsocket.h"
#include "byte.h"
#include "nanoseconds.h"
#include "pathexec.h"
#include "strtonum.h"
#include "numtostr.h"
#include "iptostr.h"
#include "porttostr.h"
#include "debug.h"
#include "warn.h"
#include "die.h"
#include "writeall.h"
#include "uint64_pack.h"
#include "statusmessage.h"
#include "blocking.h"
#include "dns.h"

#define NUMIP 8
static long long connectwait[NUMIP] = {
   1000000000LL
,  1500000000LL
,  2250000000LL
,  3375000000LL
,  5062500000LL
,  7593750000LL
, 11390625000LL
, 17085937500LL
} ;


static int flagverbose = 1;
static int flagmessage = 0;
static long long timeout = 60;

static unsigned char ip[16] = {0};
static unsigned char localip[16] = {0};
static unsigned char localport[2] = {0};
static int flaglocalip = 0;
static long long localscope = 0;
static unsigned char serverip[16 * NUMIP];
static unsigned char serverport[2];
static long long serverscope = 0;
static int s[NUMIP];
static int stype[NUMIP];
static struct pollfd p[NUMIP];
static struct pollfd *pp;

#define USAGE "\
\n\
nettcpclient: usage:\n\
\n\
 name:\n\
   nettcpclient - creates an outgoing TCP connection\n\
\n\
 syntax:\n\
   nettcpclient [options] ip port prog\n\
\n\
 description:\n\
   Nettcpclient attempts to connect to a TCP server. If it is successful, it runs prog,\n\
   with descriptor 6 reading from the network and descriptor 7 writing to the network.\n\
   Nettcpclient is similar to tcpclient (http://cr.yp.to/ucspi-tcp/tcpclient.html) and\n\
   has this differences:\n\
   - has IPv6 support\n\
   - doesn't gather remote information from DNS or from Authentication Protocol\n\
\n\
 options:\n\
   -q (optional): no error messages\n\
   -Q (optional): print error messages (default)\n\
   -v (optional): print extra information\n\
   -T timeout (optional): connect timeout (default: 60)\n\
   -i localip (optional): use localip as the IP address for the local side of the connection\n\
   -p localport (optional): use localport as the TCP port for the local side of the connection\n\
   -I iface (optional): bind to the network interface iface, IPv6 only\n\
   -m (optional): After successful connect print 32B status message to standard output\n\
   ip: server's IP address\n\
   port: server's TCP port\n\
   prog: run this program\n\
\n\
"

#define FATAL "nettcpclient: fatal: "
#define WARNING "nettcpclient: warning: "
#define DEBUG "nettcpclient: debug: "


static void warn_ipv6tcpsocket(unsigned char *ip) {

    if (flagverbose < 2) return;
    warn_4(WARNING, "unable to create IPv6 TCP socket: ", e_str(errno), "\n");
}

static void warn_connect(unsigned char *ip) {

    if (flagverbose < 2) return;
    warn_8(WARNING, "unable to connect to ", iptostr(0, ip), " ", porttostr(0, serverport), ": ", e_str(errno), "\n");
}

static void warnignored(const char *x) {
    warn_3("nettcpclient: warning: option ", x, " ignored\n");
}

static void die_usage(const char *s) {
    if (s) die_4(100, USAGE, FATAL, s, "\n");
    die_1(100, USAGE);
}


static void writemessage(long long e) {

    if (!flagmessage) return;

    if (statusmessage_write(1, e, 'T', ip, serverport) == -1)
        die_4(111, FATAL, "unable to write to stdin: ", e_str(errno), "\n");
    flagmessage = 0;
}

static void die_fatal(const char *trouble, const char *fn) {

    long long e = errno;

    if (!e) e = EPROTO;
    writemessage(e);

    if (errno) {
        if (!flagverbose) die_0(111);
        if (fn) die_7(111, FATAL, trouble, " ", fn, ": ", e_str(errno), "\n");
        die_5(111, FATAL, trouble, ": ", e_str(errno), "\n");
    }
    if (!flagverbose) die_0(111);
    if (fn) die_5(111, FATAL, trouble, " ", fn, "\n");
    die_3(111, FATAL, trouble, "\n");
}


static void die_nomem(void) {
    die_3(111, FATAL, e_str(errno), "\n");
}

int main(int argc, char **argv) {

    long long r, len, plen;
    long long  j, i, k, t;
    long long now, deadline, tm, nextaction;
    long long fd = -1;
    long long fdtype = -1;
    char *x;

    signal(SIGPIPE, SIG_IGN);

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
            if (*x == 'T') {
                if (x[1]) { strtonum(&timeout, x + 1); break; }
                if (argv[1]) { strtonum(&timeout, *++argv); break; }
            }
            if (*x == 'p') {
                if (x[1]) { if (!portparse(localport, x + 1)) die_usage("localport must be an integer between 0 and 65535"); break; }
                if (argv[1]) { if (!portparse(localport, *++argv)) die_usage("localport must be an integer between 0 and 65535"); break; }
            }

            if (*x == 'i') {
                if (x[1]) { if (!strtoip(localip, x + 1)) die_usage("localip must be a valid IP address"); flaglocalip = 1; break; }
                if (argv[1]) { if (!strtoip(localip, *++argv)) die_usage("localip must be a valid IP address"); flaglocalip = 1; break; }
            }

            if (*x == 'I') {
                if (x[1]) { serverscope = xsocket_getscopeid(x + 1); break; }
                if (argv[1]) { serverscope = xsocket_getscopeid(*++argv); break; }
            }
            if (*x == 'm') { flagmessage = 1; continue; }
            if (*x == 'M') { flagmessage = 0; continue; }

            /* not implemented */
            if (*x == '4') { warnignored("-4"); continue; }
            if (*x == '6') { warnignored("-6"); continue; }
            if (*x == 'D') { warnignored("-D"); continue; }
            if (*x == 'd') { warnignored("-d"); continue; }
            if (*x == 'H') { warnignored("-H"); continue; }
            if (*x == 'h') { warnignored("-h"); continue; }
            if (*x == 'R') { warnignored("-R"); continue; }
            if (*x == 'r') { warnignored("-r"); continue; }
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
    if (timeout <= 0) timeout = 60;
    if (timeout > 86400) timeout = 86400;

    if (!*++argv) die_usage("missing ip");
    if (!(len = strtomultiip(serverip, sizeof serverip, *argv))) die_usage("ip must be a comma-separated series of IP addresses");
    dns_sortip(serverip, len); len /= 16;
    if (!*++argv) die_usage("missing port");
    if (!portparse(serverport, *argv)) die_usage("port must be an integer between 0 and 65535");
    if (!*++argv) die_usage("missing prog");

    for (;;) {
        r = open_read("/dev/null");
        if (r == -1) die_fatal("unable to open /dev/null", 0);
        if (r > 7) { close(r); break; }
    }

    now = nanoseconds();
    deadline = timeout * 1000000000LL + now;

    for(i = 0; i < NUMIP; ++i) { s[i] = -2; stype[i] = -1; }

    k = 0; t = 0;
    for (;;) {

        now = nanoseconds();
        nextaction = deadline;
        if (deadline <= now) { errno = ETIMEDOUT; break; }

        if (k < len && s[k] == -2) {
            stype[k] = xsocket_type(serverip + 16 * k);
            s[k] = xsocket_tcp(stype[k]);
            if (s[k] == -1) { 
                if (errno != EPROTONOSUPPORT) die_fatal("unable to create TCP socket", 0);
                warn_ipv6tcpsocket(serverip + 16 * k);
                ++k;
                continue;
            }
            debug_2(flagverbose, DEBUG, "TCP socket: created\n");
            if (!flaglocalip) {
                if (stype[k] == XSOCKET_V6) byte_copy(localip, 16, xsocket_ANYIP6);
                if (stype[k] == XSOCKET_V4) byte_copy(localip, 16, xsocket_ANYIP4);
            }
            if (xsocket_bind(s[k], stype[k], localip, localport, localscope) == -1) die_fatal("unable to bind socket", 0);
            debug_6(flagverbose, DEBUG, "TCP socket: bind to ", iptostr(0, localip), " ", porttostr(0, localport), "\n");
            if (xsocket_connect(s[k], stype[k],  serverip + 16 * k, serverport, serverscope) == -1) {
                if ((errno != EWOULDBLOCK) && (errno != EINPROGRESS)) {
                    warn_connect(serverip + 16 * k);
                    close(s[k]);
                    s[k] = -1;
                    ++k;
                    continue;
                }
            }
            debug_6(flagverbose, DEBUG, "TCP connect: sent to ", iptostr(0, serverip + 16 * k), " ", porttostr(0, serverport), "\n");

            nextaction = now + connectwait[t] + fastrandommod(connectwait[t]);
            if (nextaction > deadline) nextaction = deadline;
            ++t;
        }
        ++k;


        pp = p;
        plen = 0;

        for(i = 0; i < len; ++i) {
            if (s[i] < 0) continue;
            pp->fd = s[i];
            pp->events = POLLOUT;
            ++plen; ++pp;
        }
        if (!plen) break;

        tm = (nextaction - now) / 1000000;
        if (tm <= 0) tm = 20;
        debug_4(flagverbose, DEBUG, "waiting: max ", numtostr(0, tm), " milliseconds\n");
        poll(p, plen, tm);

        now = nanoseconds();

        for(j = 0; j < plen; ++j) {
            if (!p[j].revents) continue;
            for(i = 0; i < len; ++i) {
                if (s[i] != p[j].fd) continue;
                if (xsocket_connected(p[j].fd)) {
                    fd = s[i];
                    fdtype = stype[i];
                    s[i] = -1;
                    byte_copy(ip, 16, serverip + 16 * i);
                    goto connected;
                }
                warn_connect(serverip + 16 * i);
                close(s[i]);
                s[i] = -1;
                break;
            }
        }
    }

    for(i = 0; i < len; ++i) {
        if (s[i] >= 0) {
            warn_connect(serverip + 16 * i);
            close(s[i]);
            s[i] = -1;
        }
    }
    die_fatal("unable to connect", 0);

connected:
    debug_6(flagverbose, DEBUG, "TCP connect: connected to: ", iptostr(0, ip), " ", porttostr(0, serverport), "\n");
    for(i = 0; i < len; ++i) if (s[i] >= 0) close(s[i]);
    xsocket_tcpnodelay(fd);

    if (xsocket_local(fd, fdtype, localip, localport, &localscope) == -1) die_fatal("unable to get local address", 0);

    close(6);
    if (dup(fd) != 6) die_fatal("unable to dup", 0);
    close(7);
    if (dup(fd) != 7) die_fatal("unable to dup", 0);
    close(fd);
    blocking_enable(6);
    blocking_enable(7);

    if (!pathexec_env("PROTO", "TCP")) die_nomem();
    if (!pathexec_env("REMOTEPORT", porttostr(0, serverport))) die_nomem();
    if (!pathexec_env("REMOTEIP", iptostr(0, ip))) die_nomem();
    if (!pathexec_env("LOCALPORT", porttostr(0, localport))) die_nomem();
    if (!pathexec_env("LOCALIP", iptostr(0, localip))) die_nomem();

    if (!pathexec_env("TCPREMOTEPORT", porttostr(0, serverport))) die_nomem();
    if (!pathexec_env("TCPREMOTEIP", iptostr(0, ip))) die_nomem();
    if (!pathexec_env("TCPLOCALPORT", porttostr(0, localport))) die_nomem();
    if (!pathexec_env("TCPLOCALIP", iptostr(0, localip))) die_nomem();

    signal(SIGPIPE, SIG_DFL);
    writemessage(0);
    debug_exec(flagverbose, DEBUG, "executing: ", argv);
    pathexec(argv);
    die_fatal("unable to run", *argv);
    return 111;
}
