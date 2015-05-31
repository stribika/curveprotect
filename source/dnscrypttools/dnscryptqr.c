/*
20130106
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include <poll.h>
#include "uint16_unpack_big.h"
#include "uint16_pack_big.h"
#include "die.h"
#include "str.h"
#include "byte.h"
#include "e.h"
#include "strtoip.h"
#include "printpacket.h"
#include "typeparse.h"
#include "dns.h"
#include "hexparse.h"
#include "dnscrypt.h"
#include "crypto_box.h"
#include "env.h"
#include "milliseconds.h"
#include "writeall.h"
#include "fsyncfd.h"

#define FATAL "dnscryptqr: fatal: "

#define USAGE "\
\n\
dnscryptqr: usage:\n\
\n\
 name:\n\
   dnscryptqr - asks for records of type 'type' under the domain name 'fqdn' using DNSCrypt protocol\n\
\n\
 syntax:\n\
   dnscryptqr [options] type fqdn ip [pk] [magic]\n\
\n\
 description:\n\
   dnscryptqr directly connects to the specified ip address to TCP port 443\n\
   and asks for records of type 'type' under the domain name 'fqdn' using DNS or DNSCrypt protocol.\n\
   DNSCrypt protocol is used when public-key pk is specified.\n\
   It prints the results in a human-readable format.\n\
\n\
 options:\n\
   -q (optional): no error messages\n\
   -Q (optional): print error messages (default)\n\
   -v (optional): print extra information\n\
   -h (optional): print this help\n\
\n\
 environment:\n\
   $PUBLICKEY  (optional): public-key in hexadecimal form\n\
   $SECRETKEY  (optional): secret-key in hexadecimal form\n\
   $NONCEKEY   (optional): encryption key for nonce\n\
   $NONCESTART (optional): nonce startbits\n\
\n\
 arguments:\n\
   type (mandatory): DNS record type (a, mx, ns, soa, txt, ...)\n\
   fqdn (mandatory): DNS domain name \n\
   ip   (mandatory): IPv4 address\n\
   pk    (optional): DNSCrypt server public-key (Curve25519 public-key in hex format)\n\
   magic (optional): DNSCrypt protocol query magic (default: q6fnvWj9)\n\
\n\
 example:\n\
   dnscryptqr ns opendns.com 208.67.220.220\n\
\n\
\
"


static int flagverbose = 1;


void die_usage(const char *s) {
    if (!flagverbose) die_0(100);
    if (s) die_4(100, USAGE, FATAL, s, "\n");
    die_1(100, USAGE);
}

static void die_fatal(const char *trouble, const char *fn) {

    if (errno) {
        if (fn) die_7(111, FATAL, trouble, " ", fn, ": ", e_str(errno), "\n");
        die_5(111, FATAL, trouble, ": ", e_str(errno), "\n");
    }
    if (fn) die_5(111, FATAL, trouble, " ", fn, "\n");
    die_3(111, FATAL, trouble, "\n");
}

static void oops(void) {
    die_fatal("unable to parse", 0);
}

static struct dns_transmit tx;
static unsigned char servers[256];
static unsigned char keys[528];
static unsigned char pk[crypto_box_PUBLICKEYBYTES];
static unsigned char sk[crypto_box_SECRETKEYBYTES];
static int flagpk = 0;
static long long maxtimeout = 60;

static unsigned char type[2];
static unsigned char *q;

static unsigned char port[2];
static char *transport = "regular DNS";

static int resolve(void) {

    long long deadline, stamp, timeout, max;
    struct pollfd x[1];
    int r;

    uint16_pack_big(port, 443);

    if (flagpk) {
        if (dns_transmit_startext(&tx, servers, 1, 1, q, type, 0, port, keys, pk, 0) == -1) return -1;
    }
    else {
        if (dns_transmit_startext(&tx, servers, 1, 1, q, type, 0, port, 0, 0, 0) == -1) return -1;
    }

    max = maxtimeout * 1000 + milliseconds();

    for (;;) {
        stamp = milliseconds();
        if (stamp > max) {
            errno = ETIMEDOUT;
            dns_verbosity_queryfailed(&tx, 1);
            return -1;
        }
        deadline = max;
        dns_transmit_io(&tx, x, &deadline);
        timeout = deadline - stamp;
        if (timeout <= 0) timeout = 20;
        poll(x, 1, timeout);
        r = dns_transmit_get(&tx, x, stamp);
        if (r == -1) return -1;
        if (r == 1) break;
    }
    return 0;
}

static stralloc out;
static unsigned char nk[16];

char *query_magic = DNSCRYPT_QUERY_MAGIC;
char *response_magic = DNSCRYPT_RESPONSE_MAGIC;

int main(int argc, char **argv) {

    crypto_uint16 u16;
    char *x;

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
            if (*x == 'v') { if (flagverbose == 2) flagverbose = 3; else flagverbose = 2; continue; }
            if (*x == 'h') die_usage(0);
            die_usage(0);
        }
    }
    if (!*++argv) die_usage("type not set");
    if (!typeparse(type, *argv)) die_usage("unable to parse type");
    if (!*++argv) die_usage("fqdn not set");
    if (!dns_domain_fromdot(&q, (unsigned char *)*argv, str_len(*argv))) die_usage("fqdn must be at most 255 bytes, at most 63 bytes between dots");
    if (!*++argv) die_usage("ip not set");
    byte_zero(servers,sizeof servers);
    if (!strtoip(servers, *argv)) die_usage("unable to parse ip address");
    if (hexparse(keys + 1, crypto_box_PUBLICKEYBYTES, *++argv)) flagpk = 1;

    x = *++argv;
    if (x && str_len(x) == 8) query_magic = x;

    if (flagpk) {

      if (!hexparse(sk, sizeof sk, env_get("SECRETKEY")) ||
          !hexparse(pk, sizeof pk, env_get("PUBLICKEY")) ||
          !hexparse(nk, sizeof nk, env_get("NONCEKEY"))) {
        dns_keys(pk, sk, nk);
      }
      dns_nonce_init(env_get("NONCESTART"), nk);
      crypto_box_beforenm(keys + 1, keys + 1, sk);
      keys[0] = 1;
      dns_transmit_magic(query_magic, response_magic);
      transport = "DNSCrypt";
    }

    if (!stralloc_copys(&out, "")) oops();
    u16 = uint16_unpack_big(type);
    if (!stralloc_catnum(&out, u16)) oops();
    if (!stralloc_cats(&out, " ")) oops();
    if (!dns_domain_todot_cat(&out, q)) oops();
    if (!stralloc_cats(&out, " - ")) oops();
    if (!stralloc_cats(&out, transport)) oops();
    if (!stralloc_cats(&out, ":\n")) oops();

    if (resolve() == -1) {
        if (!stralloc_cats(&out, e_str(errno))) oops();
        if (!stralloc_cats(&out, "\n")) oops();
    }
    else {
        if (tx.packetlen < 4) oops();
        tx.packet[2] &= ~1;
        tx.packet[3] &= ~128;
        if (!printpacket_cat(&out, tx.packet, tx.packetlen)) oops(); 
    }

    if (writeall(1, out.s, out.len) == -1) die_fatal("unable to write output", 0);
    if (fsyncfd(1) == -1) die_fatal("unable to write output", 0);
    _exit(0);
}
