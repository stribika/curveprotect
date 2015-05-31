/*
20121229
Jan Mojzis
Public domain.
*/

#include <poll.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include "stralloc.h"
#include "uint16_pack_big.h"
#include "uint16_unpack_big.h"
#include "crypto_uint32.h"
#include "byte.h"
#include "dns.h"
#include "die.h"
#include "warn.h"
#include "e.h"
#include "str.h"
#include "numtostr.h"
#include "strtoip.h"
#include "hexparse.h"
#include "providerparse.h"
#include "dnscrypt.h"
#include "fsyncfd.h"
#include "buffer.h"
#include "milliseconds.h"
#include "strtomultiip.h"
#include "crypto_sign_ed25519.h"

#define FATAL "dnscryptgetcert: fatal: "

#define USAGE "\
\n\
dnscryptgetcert: usage:\n\
\n\
 name:\n\
   dnscryptgetcert - get DNSCrypt certificate\n\
\n\
 syntax:\n\
   dnscryptgetcert [options] provider pk ip\n\
\n\
 description:\n\
   dnscryptgetcert directly connects to the specified ip address to TCP port 443\n\
   and issues a TXT query for the provider, gets DNSCrypt certificate and prints it in format:\n\
   serial-number server-public-key query-magic valid-since valid-to\n\
\n\
 options:\n\
   -q (optional): no error messages\n\
   -Q (optional): print error messages (default)\n\
   -v (optional): print extra information\n\
   -h (optional): print this help\n\
\n\
 arguments:\n\
   provider (mandatory): DNSCrypt provider name\n\
   pk       (mandatory): DNSCrypt provider public-key (Ed25519 public-key in hex format)\n\
   ip       (mandatory): a comma-separated series of IPv4 addresses\n\
\n\
 example:\n\
   dnscryptgetcert 2.dnscrypt-cert.opendns.com B7351140206F225D3E2BD822D7FD691EA1C33CC8D6668D0CBE04BFABCA43FB79 208.67.220.220,208.67.222.222\n\
\
"

static const char *name = 0;
static unsigned char providerpk[32];
static stralloc providername = {0};

static stralloc tmp = {0};

static int flagok = 0;
static int flagverbose = 1;

static void die_usage(const char *s) {
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

static void printabletmp(void) {

    long long i;

    for (i = 0; i < tmp.len; ++i) {
        if (tmp.s[i] < 32) tmp.s[i] = '?';
        if (tmp.s[i] > 126) tmp.s[i] = '?';
    }
    tmp.s[tmp.len] = 0;
}

static void warn(const char *err) {

    if (flagverbose < 2) return;
    printabletmp();
    warn_4("dnscryptgetcert: warning: unable to find DNSCrypt certificate in TXT record '", (char *)tmp.s, "': ", err);
}



static void out(const char *x, long long len) {
    if (buffer_put(buffer_1, x, len) == -1) die_fatal("unable to write output", 0);
}

static void outs(const char *x) {
    if (buffer_puts(buffer_1, x) == -1) die_fatal("unable to write output", 0);
}

static void outnum(long long num, long long len) {

    char *x;
    x = numtostr(0, num);
    len -= str_len(x);

    while (len-- > 0) {
        if (buffer_put(buffer_1, "0", 1) == -1) die_fatal("unable to write output", 0);
    }
    if (buffer_puts(buffer_1, x) == -1) die_fatal("unable to write output", 0);
}

static void flush(void) {
    if (buffer_flush(buffer_1) == -1) die_fatal("unable to write output", 0);
}

static int parse(void) {

    crypto_uint32 serial, periodsince, periodto;
    long long i;
    char ch;

    const char *err;
    char query_magic[8];
    unsigned char serverpk[32];

    struct tm *t;
    time_t secs;

    if (tmp.len == 0) return 0;
    err = dnscrypt_unpack(serverpk, query_magic, &serial, &periodsince, &periodto, providerpk, tmp.s, tmp.len);
    if (err) {
        warn(err);
        return 0;
    }
    
    /* print serial */
    outnum(serial, 10);
    outs(" ");

    /* print pk */
    for (i = 0;i < 32; ++i) {
      ch = "0123456789abcdef"[15 & (int) (serverpk[i] >> 4)];
      out(&ch, 1);
      ch = "0123456789abcdef"[15 & (int) (serverpk[i] >> 0)];
      out(&ch, 1);
    }
    outs(" ");

    /* print magic */
    for (i = 0; i < 8; ++i) {
      ch = query_magic[i];
      if (ch < 32) ch = '?';
      if (ch > 126) ch = '?';
      out(&ch, 1);
    }
    outs(" ");

    /* print since - to */
    outnum(periodsince, 0);
    outs(" ");
    outnum(periodto, 0);
    outs("\n");

    if (flagverbose >= 2) {

        secs = periodsince;
        t = gmtime(&secs);
        outs("... certificate valid since:");
        out(" ",1); outnum(t->tm_hour,2);
        out(":",1); outnum(t->tm_min,2);
        out(":",1); outnum(t->tm_sec,2);
        out(" ",1); outnum(t->tm_mday,2);
        out(".",1); outnum(1 + t->tm_mon,2);
        out(".",1); outnum(1900 + t->tm_year,0);
        outs(" GMT\n");

        secs = periodto;
        t = gmtime(&secs);
        outs("... certificate valid to:   ");
        out(" ",1); outnum(t->tm_hour,2);
        out(":",1); outnum(t->tm_min,2);
        out(":",1); outnum(t->tm_sec,2);
        out(" ",1); outnum(t->tm_mday,2);
        out(".",1); outnum(1 + t->tm_mon,2);
        out(".",1); outnum(1900 + t->tm_year,0);
        outs(" GMT\n");
    }

    flush();
    flagok = 1;
    return 0;

}

static struct dns_transmit dns_dnscryptgetcert_resolve_tx = {0};
static unsigned char servers[256];
static long long maxtimeout = 60;
static unsigned char port[2];

static int dns_dnscryptgetcert_resolve(const unsigned char *q, const unsigned char qtype[2]) {

    long long deadline, stamp, timeout, max;
    struct pollfd x[1];
    int r;

    uint16_pack_big(port, 443);

    if (dns_transmit_startext(&dns_dnscryptgetcert_resolve_tx, servers, 1, 1, q, qtype, 0, port, 0, 0, 0) == -1) return -1;

    max = maxtimeout * 1000 + milliseconds();

    for (;;) {
        stamp = milliseconds();
        if (stamp > max) {
            errno = ETIMEDOUT;
            dns_verbosity_queryfailed(&dns_dnscryptgetcert_resolve_tx, 1);
            return -1;
        }
        deadline = max;
        dns_transmit_io(&dns_dnscryptgetcert_resolve_tx, x, &deadline);
        timeout = deadline - stamp;
        if (timeout <= 0) timeout = 20;
        poll(x, 1, timeout);
        r = dns_transmit_get(&dns_dnscryptgetcert_resolve_tx, x, stamp);
        if (r == -1) return -1;
        if (r == 1) break;
    }
    return 0;
}

static int dns_dnscryptgetcert_packet(const unsigned char *buf,long long len)
{
  long long pos;
  unsigned char header[12];
  crypto_uint16 numanswers;
  crypto_uint16 datalen;
  unsigned char ch;
  long long txtlen;
  long long  i;

  if (!stralloc_copys(&tmp,"")) return -1;

  pos = dns_packet_copy(buf,len,0,header,12); if (!pos) return -1;
  numanswers = uint16_unpack_big(header + 6);
  pos = dns_packet_skipname(buf,len,pos); if (!pos) return -1;
  pos += 4;

  while (numanswers--) {
    pos = dns_packet_skipname(buf,len,pos); if (!pos) return -1;
    pos = dns_packet_copy(buf,len,pos,header,10); if (!pos) return -1;
    datalen = uint16_unpack_big(header + 8);
    if (byte_isequal(header,2,DNS_T_TXT))
      if (byte_isequal(header + 2,2,DNS_C_IN)) {
	if (pos + datalen > len) return -1;
	txtlen = 0;
	for (i = 0;i < datalen;++i) {
	  ch = buf[pos + i];
	  if (!txtlen) {
            if (parse() == -1) return -1;
	    txtlen = ch;
            if (!stralloc_copys(&tmp,"")) return -1;
          }
	  else {
	    --txtlen;
	    if (!stralloc_append(&tmp,&ch)) return -1;
	  }
	}
      }
    pos += datalen;
  }

  if (parse() == -1) return -1;
  return 0;
}

static unsigned char *q = 0;

static int dns_dnscryptgetcert(void)
{
  if (dns_dnscryptgetcert_resolve(q,DNS_T_TXT) == -1) return -1;
  if (dns_dnscryptgetcert_packet(dns_dnscryptgetcert_resolve_tx.packet,dns_dnscryptgetcert_resolve_tx.packetlen) == -1) return -1;
  dns_transmit_free(&dns_dnscryptgetcert_resolve_tx);
  return 0;
}

int main(int argc, char **argv) {

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
    name = *++argv;
    if (!providerparse(&providername, &q, *argv)) {
        if (errno == ENOMEM) die_fatal("out of memory", 0);
        die_usage("provider must be at most 255 bytes, at most 63 bytes between dots");
    }
    if (!hexparse(providerpk, sizeof providerpk, *++argv)) die_usage("pk must be exactly 64 hex characters");
    if (!strtomultiip(servers, sizeof servers, *++argv)) die_usage("ip must be a comma-separated series of IPv4 addresses");

    if (dns_dnscryptgetcert() == -1) die_fatal("unable to find TXT records", 0);
    if (!flagok) { errno = 0; die_fatal("unable to find DNSCrypt signed certificate:", "txt record not found"); }

    if (fsyncfd(1) == -1) die_fatal("unable to write output", 0);
    _exit(0);
}
