/*
 * 20130507
 * Jan Mojzis
 * Public domain.
 */

#include "dns.h"
#include "iptostr.h"
#include "e.h"
#include "pathexec.h"
#include "alloc.h"
#include "die.h"
#include "debug.h"
#include "byte.h"
#include "statusmessage.h"

#define USAGE "\
\n\
netclient: usage:\n\
\n\
 name:\n\
   netclient - creates an outgoing CurveCP or TCP connection.\n\
\n\
 syntax:\n\
   netclient [options] host port prog\n\
\n\
 description:\n\
   Netclient attempts to connect to a TCP or CurveCP server. If it is successful, it runs prog,\n\
   with descriptor 6 reading from the network and descriptor 7 writing to the network.\n\
\n\
 options:\n\
   -q (optional): no error messages\n\
   -Q (optional): print error messages (default)\n\
   -v (optional): print extra information\n\
   -4 (optional): IPv4 only - skip AAAA DNS queries\n\
   -6 (optional): IPv6 only - skip A DNS queries\n\
   -u (optional): allow TCP connection - allow CurveCP + TCP (default)\n\
   -U (optional): deny TCP connection - allow only CurveCP\n\
   -c (optional): program is a client; server starts first, CurveCP only (default)\n\
   -C (optional): program is a client that starts first, CurveCP only\n\
   -s (optional): service selector, CurveCP only\n\
   -k keydir (optional): use this public-key directory, CurveCP only\n\
   -T timeout (optional): connect timeout (default 60)\n\
   -I iface (optional): bind to the network interface iface, IPv6 only\n\
   -m (optional): After successful connect print 32B status message to standard output\n\
   ip: server's IP address\n\
   port: server's TCP/UDP port\n\
   prog: run this program\n\
\n\
"

#define FATAL "netclient: fatal: "
#define WARNING "netclient: warning: "
#define DEBUG "netclient: debug: "

static int flagunencrypted = 1;
static int flagmessage = 0;
static int flagcfirst = 0;
static char *keydir = 0;
static char *host = 0;
static char *port = 0;
static char *timeout = "60";
static char *localinterface = 0;
static struct dns_data r = {0};

static char hexkey[65];
static char hexext[33];

static int flagverbose = 1;

static int (*dns_ipq_op)(struct dns_data *, const char *) = dns_ip_qualify;


static void die_usage(const char *s) {
    if (s) die_4(100, USAGE, FATAL, s, "\n");
    die_1(100, USAGE);
}


static unsigned char zero[16] = {0};
static void writemessage(long long e) {

    if (!flagmessage) return;

    if (statusmessage_write(1, e, 'X', zero, zero) == -1)
        die_4(111, FATAL, "unable to write to stdin: ", e_str(errno), "\n");
    flagmessage = 0;
}


static void die_fatal(const char *trouble, const char *fn) {

    long long e = errno;

    if (!e) e = EPROTO;
    writemessage(e);

    if (!flagverbose) die_0(111);

    if (errno) {
        if (fn) die_7(111, FATAL, trouble, " ", fn, ": ", e_str(errno), "\n");
        die_5(111, FATAL, trouble, ": ", e_str(errno), "\n");
    }
    if (fn) die_5(111, FATAL, trouble, " ", fn, "\n");
    die_3(111, FATAL, trouble, "\n");
}

static void die_nomem(void) {
    die_3(errno, FATAL, e_str(errno), "\n");
}

static char ips[512] = {0};
static long long ipslen = 0;

static void ip_add(unsigned char *y) {

    long long len = ipslen - 1;

    char *x;

    x = iptostr(0, y);

    while (*x) {
        if (ipslen == sizeof ips) return;
        ips[ipslen++] = *x++;
    }
    if (ipslen == sizeof ips) return;
    ips[ipslen++] = 0;
    if (len > 0) ips[len] = ',';
}

int main(int argc, char **argv, char **envp) {

    char *x;
    long long i, j;
    char **run;

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
            if (*x == '4') { dns_ipq_op = dns_ip4_qualify; continue; }
            if (*x == '6') { dns_ipq_op = dns_ip6_qualify; continue; }
            if (*x == 'u') { flagunencrypted = 1; continue; }
            if (*x == 'U') { flagunencrypted = 0; continue; }
            if (*x == 'c') { flagcfirst = 0; continue; }
            if (*x == 'C') { flagcfirst = 1; continue; }
            if (*x == 'm') { flagmessage = 1; continue; }
            if (*x == 'M') { flagmessage = 0; continue; }
            if (*x == 'k') {
                if (x[1]) { keydir = x + 1; break; }
                if (argv[1]) { keydir = *++argv; break; }
            }
            if (*x == 'T') {
                if (x[1]) { timeout = x + 1; break; }
                if (argv[1]) { timeout = *++argv; break; }
            }
            if (*x == 's') {
                if (x[1]) { r.curvecpselector = x[1]; break; }
                if (argv[1]) { ++argv; r.curvecpselector = *++argv[0]; break; }
            }
            if (*x == 'I') {
                if (x[1]) { localinterface = x + 1; break; }
                if (argv[1]) { localinterface = *++argv; break; }
            }
            die_usage(0);
        }
    }
    errno = 0;
    if (!*++argv) die_usage("missing host"); host = *argv;
    if (!*++argv) die_usage("missing port"); port = *argv;
    if (!*++argv) die_usage("missing prog");

    dns_verbosity_setflag(flagverbose);
    dns_verbosity_setmessage(DEBUG);

    if (dns_ipq_op(&r, host) == -1) die_fatal("temporarily unable to figure out IP address for", host);
    if (r.result.len == 0) { errno = DNSNOENT; die_fatal("no IP address for", host); }
    host = (char *)r.fqdn.s;

    for (j = 0; j + 16 <= r.result.len; j += 16) {
        ip_add(r.result.s + j);
    }
    if (ipslen == 0) { errno = 0; die_fatal("no IP address for", host); }

    run = (char **)alloc((argc + 20) * (sizeof (char **)));
    if (!run) die_nomem();

    i = 0;
    if (!r.curvecpkey) {
        debug_2(flagverbose, DEBUG, "transport: TCP\n");
        if (!flagunencrypted) { errno = 0; die_fatal("TCP transport not allowed", 0); }
        run[i++] = "nettcpclient";
        if (timeout) {
            run[i++] = "-T";
            run[i++] = timeout;
        }
        if (localinterface) {
            run[i++] = "-I";
            run[i++] = localinterface;
        }
        if (flagmessage) run[i++] = "-m";
        if (flagverbose < 1) run[i++] = "-q";
        else if (flagverbose > 2) run[i++] = "-vv";
        else if (flagverbose > 1) run[i++] = "-v";
        run[i++] = ips;
        run[i++] = port;
    }
    else {
        for (j = 0; j < 32; ++j) {
            hexkey[2 * j] = "0123456789abcdef"[r.curvecpkey[j] >> 4];
            hexkey[2 * j + 1] = "0123456789abcdef"[r.curvecpkey[j] & 15];
        }
        hexkey[64] = 0;
        for (j = 0; j < 16; ++j) {
            hexext[2 * j] = "0123456789abcdef"[r.curvecpkey[j + 32] >> 4];
            hexext[2 * j + 1] = "0123456789abcdef"[r.curvecpkey[j + 32] & 15];
        }
        hexext[32] = 0;
        debug_2(flagverbose, DEBUG, "transport: CurveCP\n");
        debug_4(flagverbose, DEBUG, "CurveCP public-key: ", hexkey, "\n");
        debug_4(flagverbose, DEBUG, "CurveCP extension: ", hexext, "\n");

        run[i++] = "netcurvecpclient";
        if (flagverbose < 1) run[i++] = "-q";
        else if (flagverbose > 2) run[i++] = "-vv";
        else if (flagverbose > 1) run[i++] = "-v";
        if (keydir) {
            run[i++] = "-c";
            run[i++] = keydir;
        }
        if (flagmessage) run[i++] = "-m";
        run[i++] = host;
        run[i++] = hexkey;
        run[i++] = ips;
        run[i++] = port;
        run[i++] = hexext;
        run[i++] = "netcurvecpmessage";
        if (flagcfirst) {
            run[i++] = "-C";
        }
        else {
            run[i++] = "-c";
        }
    }

    for(j = 0; j < argc; ++j) {
        run[i++] = argv[j];
    }
    run[i++] = 0;

    debug_exec(flagverbose, DEBUG, "executing: ", run);
    pathexec_run(*run, run, envp);
    die_fatal("unable to run", *run);
    return 111;
}
