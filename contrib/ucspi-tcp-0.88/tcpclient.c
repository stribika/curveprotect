#include <sys/types.h>
#include <sys/param.h>
#include <netdb.h>
#include "sig.h"
#include "exit.h"
#include "sgetopt.h"
#include "uint16.h"
#include "fmt.h"
#include "scan.h"
#include "str.h"
#include "ip4.h"
#include "uint16.h"
#include "socket.h"
#include "fd.h"
#include "stralloc.h"
#include "buffer.h"
#include "error.h"
#include "strerr.h"
#include "pathexec.h"
#include "timeoutconn.h"
#include "remoteinfo.h"
#include "dns.h"

#define FATAL "tcpclient: fatal: "
#define CONNECT "tcpclient: unable to connect to "

void nomem(void)
{
  strerr_die2x(111,FATAL,"out of memory");
}
void usage(void)
{
  strerr_die1x(100,"tcpclient: usage: tcpclient \
[ -hHrRdDqQv ] \
[ -i localip ] \
[ -p localport ] \
[ -T timeoutconn ] \
[ -l localname ] \
[ -t timeoutinfo ] \
host port program");
}

int verbosity = 1;
int flagdelay = 1;
int flagremoteinfo = 1;
int flagremotehost = 1;
unsigned long itimeout = 26;
unsigned long ctimeout[2] = { 2, 58 };

char iplocal[4] = { 0,0,0,0 };
uint16 portlocal = 0;
char *forcelocal = 0;

char ipremote[4];
uint16 portremote;

char *hostname;
static stralloc addresses;
static stralloc moreaddresses;

static stralloc tmp;
static stralloc fqdn;
char strnum[FMT_ULONG];
char ipstr[IP4_FMT];

char seed[128];

main(int argc,char **argv)
{
  unsigned long u;
  int opt;
  char *x;
  int j;
  int s;
  int cloop;

  dns_random_init(seed);

  close(6);
  close(7);
  sig_ignore(sig_pipe);
 
  while ((opt = getopt(argc,argv,"dDvqQhHrRi:p:t:T:l:")) != opteof)
    switch(opt) {
      case 'd': flagdelay = 1; break;
      case 'D': flagdelay = 0; break;
      case 'v': verbosity = 2; break;
      case 'q': verbosity = 0; break;
      case 'Q': verbosity = 1; break;
      case 'l': forcelocal = optarg; break;
      case 'H': flagremotehost = 0; break;
      case 'h': flagremotehost = 1; break;
      case 'R': flagremoteinfo = 0; break;
      case 'r': flagremoteinfo = 1; break;
      case 't': scan_ulong(optarg,&itimeout); break;
      case 'T': j = scan_ulong(optarg,&ctimeout[0]);
		if (optarg[j] == '+') ++j;
		scan_ulong(optarg + j,&ctimeout[1]);
		break;
      case 'i': if (!ip4_scan(optarg,iplocal)) usage(); break;
      case 'p': scan_ulong(optarg,&u); portlocal = u; break;
      default: usage();
    }
  argv += optind;

  if (!verbosity)
    buffer_2->fd = -1;

  hostname = *argv;
  if (!hostname) usage();
  if (str_equal(hostname,"")) hostname = "127.0.0.1";
  if (str_equal(hostname,"0")) hostname = "127.0.0.1";

  x = *++argv;
  if (!x) usage();
  if (!x[scan_ulong(x,&u)])
    portremote = u;
  else {
    struct servent *se;
    se = getservbyname(x,"tcp");
    if (!se)
      strerr_die3x(111,FATAL,"unable to figure out port number for ",x);
    portremote = ntohs(se->s_port);
    /* i continue to be amazed at the stupidity of the s_port interface */
  }

  if (!*++argv) usage();

  if (!stralloc_copys(&tmp,hostname)) nomem();
  if (dns_ip4_qualify(&addresses,&fqdn,&tmp) == -1)
    strerr_die4sys(111,FATAL,"temporarily unable to figure out IP address for ",hostname,": ");
  if (addresses.len < 4)
    strerr_die3x(111,FATAL,"no IP address for ",hostname);

  if (addresses.len == 4) {
    ctimeout[0] += ctimeout[1];
    ctimeout[1] = 0;
  }

  for (cloop = 0;cloop < 2;++cloop) {
    if (!stralloc_copys(&moreaddresses,"")) nomem();
    for (j = 0;j + 4 <= addresses.len;j += 4) {
      s = socket_tcp();
      if (s == -1)
        strerr_die2sys(111,FATAL,"unable to create socket: ");
      if (socket_bind4(s,iplocal,portlocal) == -1)
        strerr_die2sys(111,FATAL,"unable to bind socket: ");
      if (timeoutconn(s,addresses.s + j,portremote,ctimeout[cloop]) == 0)
        goto CONNECTED;
      close(s);
      if (!cloop && ctimeout[1] && (errno == error_timeout)) {
	if (!stralloc_catb(&moreaddresses,addresses.s + j,4)) nomem();
      }
      else {
        strnum[fmt_ulong(strnum,portremote)] = 0;
        ipstr[ip4_fmt(ipstr,addresses.s + j)] = 0;
        strerr_warn5(CONNECT,ipstr," port ",strnum,": ",&strerr_sys);
      }
    }
    if (!stralloc_copy(&addresses,&moreaddresses)) nomem();
  }

  _exit(111);



  CONNECTED:

  if (!flagdelay)
    socket_tcpnodelay(s); /* if it fails, bummer */

  if (!pathexec_env("PROTO","TCP")) nomem();

  if (socket_local4(s,iplocal,&portlocal) == -1)
    strerr_die2sys(111,FATAL,"unable to get local address: ");

  strnum[fmt_ulong(strnum,portlocal)] = 0;
  if (!pathexec_env("TCPLOCALPORT",strnum)) nomem();
  ipstr[ip4_fmt(ipstr,iplocal)] = 0;
  if (!pathexec_env("TCPLOCALIP",ipstr)) nomem();

  x = forcelocal;
  if (!x)
    if (dns_name4(&tmp,iplocal) == 0) {
      if (!stralloc_0(&tmp)) nomem();
      x = tmp.s;
    }
  if (!pathexec_env("TCPLOCALHOST",x)) nomem();

  if (socket_remote4(s,ipremote,&portremote) == -1)
    strerr_die2sys(111,FATAL,"unable to get remote address: ");

  strnum[fmt_ulong(strnum,portremote)] = 0;
  if (!pathexec_env("TCPREMOTEPORT",strnum)) nomem();
  ipstr[ip4_fmt(ipstr,ipremote)] = 0;
  if (!pathexec_env("TCPREMOTEIP",ipstr)) nomem();
  if (verbosity >= 2)
    strerr_warn4("tcpclient: connected to ",ipstr," port ",strnum,0);

  x = 0;
  if (flagremotehost)
    if (dns_name4(&tmp,ipremote) == 0) {
      if (!stralloc_0(&tmp)) nomem();
      x = tmp.s;
    }
  if (!pathexec_env("TCPREMOTEHOST",x)) nomem();

  x = 0;
  if (flagremoteinfo)
    if (remoteinfo(&tmp,ipremote,portremote,iplocal,portlocal,itimeout) == 0) {
      if (!stralloc_0(&tmp)) nomem();
      x = tmp.s;
    }
  if (!pathexec_env("TCPREMOTEINFO",x)) nomem();

  if (fd_move(6,s) == -1)
    strerr_die2sys(111,FATAL,"unable to set up descriptor 6: ");
  if (fd_copy(7,6) == -1)
    strerr_die2sys(111,FATAL,"unable to set up descriptor 7: ");
  sig_uncatch(sig_pipe);
 
  pathexec(argv);
  strerr_die4sys(111,FATAL,"unable to run ",*argv,": ");
}
