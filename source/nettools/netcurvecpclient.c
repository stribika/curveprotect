#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include "e.h"
#include "die.h"
#include "debug.h"
#include "load.h"
#include "open.h"
#include "byte.h"
#include "xsocket.h"
#include "uint64_pack.h"
#include "uint64_unpack.h"
#include "nanoseconds.h"
#include "hexparse.h"
#include "nameparse.h"
#include "portparse.h"
#include "writeall.h"
#include "safenonce.h"
#include "fastrandommod.h"
#include "strtomultiip.h"
#include "statusmessage.h"
#include "iptostr.h"
#include "numtostr.h"

long long recent = 0;

#define NUMIP 5
long long hellowait[NUMIP + 3] = {
   1000000000LL
,  1500000000LL
,  2250000000LL
,  3375000000LL
,  5062500000LL
,  7593750000LL
, 11390625000LL
, 17085937500LL
} ;

#include "crypto_box.h"
#include "fastrandombytes.h"
#if crypto_box_PUBLICKEYBYTES != 32
error!
#endif
#if crypto_box_NONCEBYTES != 24
error!
#endif
#if crypto_box_BOXZEROBYTES != 16
error!
#endif
#if crypto_box_ZEROBYTES != 32
error!
#endif
#if crypto_box_BEFORENMBYTES != 32
error!
#endif

int flagverbose = 1;
int flagmessage = 0;

#define USAGE "\
netcurvecpclient: how to use:\n\
netcurvecpclient:   -q (optional): no error messages\n\
netcurvecpclient:   -Q (optional): print error messages (default)\n\
netcurvecpclient:   -v (optional): print extra information\n\
netcurvecpclient:   -c keydir (optional): use this public-key directory\n\
netcurvecpclient:   sname: server's name\n\
netcurvecpclient:   pk: server's public key\n\
netcurvecpclient:   ip: server's IP address\n\
netcurvecpclient:   port: server's UDP port\n\
netcurvecpclient:   ext: server's extension\n\
netcurvecpclient:   prog: run this client\n\
"

void die_usage(const char *s)
{
  if (s) die_4(100,USAGE,"netcurvecpclient: fatal: ",s,"\n");
  die_1(100,USAGE);
}


unsigned char packetip[16];
unsigned char packetport[2];
static void writemessage(long long e) {

    if (!flagmessage) return;

    if (statusmessage_write(1, e, 'C', packetip, packetport) == -1)
        die_3(111, "netcurvecpclient: fatal: unable to write to stdin: ", e_str(errno), "\n");
    flagmessage = 0;
}

void die_fatal(const char *trouble,const char *d,const char *fn)
{

  long long e = errno;

  if (!e) e = EPROTO;
  writemessage(e);


  /* XXX: clean up? OS can do it much more reliably */
  if (!flagverbose) die_0(111);
  if (d) {
    if (fn) die_9(111,"netcurvecpclient: fatal: ",trouble," ",d,"/",fn,": ",e_str(errno),"\n");
    die_7(111,"netcurvecpclient: fatal: ",trouble," ",d,": ",e_str(errno),"\n");
  }
  if (errno) die_5(111,"netcurvecpclient: fatal: ",trouble,": ",e_str(errno),"\n");
  die_3(111,"netcurvecpclient: fatal: ",trouble,"\n");
}


static void sortip(unsigned char *s, long long nn) {

    long long i;
    unsigned char tmp[16];
    long long n = nn;

    n >>= 4;
    while (n > 1) {
        i = fastrandommod(n);
        --n;
        byte_copy(tmp, 16, s + (i << 4));
        byte_copy(s + (i << 4), 16, s + (n << 4));
        byte_copy(s + (n << 4), 16, tmp);
    }

    for (i = 0; i + 16 <= nn; i += 16) {
        if (!byte_isequal(s + i, 12, "\0\0\0\0\0\0\0\0\0\0\377\377")) {
            byte_copy(tmp, 16, s + i);
            byte_copy(s + i, 16, s);
            byte_copy(s, 16, tmp);
            break;
        }
    }
}

int multiipparse(unsigned char *y, long long ylen, const char *x)
{
  long long ynum;
  long long pos;
  long long pos2;

  if (!x) return 0;

  ynum = strtomultiip(y, ylen, x);
  if (ynum == 0) return 0;
  sortip(y, ynum);

  /* if fewer than 8 IP addresses, cycle through them: */
  pos = 0;
  pos2 = ynum;
  while (pos2 < ylen) {
    if (pos >= 0 && pos < ylen && pos2 >= 0 && pos2 < ylen) y[pos2] = y[pos];
    ++pos2;
    ++pos;
  }
  return 1;
}


/* routing to the client: */
unsigned char clientextension[16];
long long clientextensionloadtime = 0;
int udpfd = -1;
int udpfd4 = -1;
int udpfd6 = -1;
int udpfdtype = -1;

void clientextension_init(void)
{
  if (recent >= clientextensionloadtime) {
    clientextensionloadtime = recent + 30000000000LL;
    if (load("/etc/curvecpextension",clientextension,16) == -1)
      if (errno == ENOENT || errno == ENAMETOOLONG)
        byte_zero(clientextension,16);
  }
}


/* client security: */
char *keydir = 0;
unsigned char clientlongtermpk[32];
unsigned char clientlongtermsk[32];
unsigned char clientshorttermpk[32];
unsigned char clientshorttermsk[32];
crypto_uint64 clientshorttermnonce;
unsigned char vouch[64];

void clientshorttermnonce_update(void)
{
  ++clientshorttermnonce;
  if (clientshorttermnonce) return;
  errno = EPROTO;
  die_fatal("nonce space expired",0,0);
}

/* routing to the server: */
unsigned char serverip[16 * NUMIP];
unsigned char serverport[2];
unsigned char serverextension[16];

/* server security: */
unsigned char servername[256];
unsigned char serverlongtermpk[32];
unsigned char servershorttermpk[32];
unsigned char servercookie[96];

/* shared secrets: */
unsigned char clientshortserverlong[32];
unsigned char clientshortservershort[32];
unsigned char clientlongserverlong[32];

unsigned char allzero[128] = {0};

unsigned char nonce[24];
unsigned char text[2048];

unsigned char packet[4096];
/*unsigned char packetip[16];
unsigned char packetport[2];*/
crypto_uint64 packetnonce;
int flagreceivedmessage = 0;
crypto_uint64 receivednonce = 0;

struct pollfd p[3];

int fdwd = -1;

int tochild[2] = {-1,-1};
int fromchild[2] = {-1,-1};
pid_t child = -1;
int childstatus = 0;

unsigned char childbuf[4096];
long long childbuflen = 0;
unsigned char childmessage[2048];
long long childmessagelen = 0;

void exitasap(int sig) {
  close(0);
}

int main(int argc,char **argv)
{
  long long hellopackets;
  long long r;
  long long nextaction;

  signal(SIGPIPE,SIG_IGN);
  signal(SIGHUP, exitasap);
  signal(SIGTERM, exitasap);
  signal(SIGINT, exitasap);


  if (!argv[0]) die_usage(0);
  for (;;) {
    char *x;
    if (!argv[1]) break;
    if (argv[1][0] != '-') break;
    x = *++argv;
    if (x[0] == '-' && x[1] == 0) break;
    if (x[0] == '-' && x[1] == '-' && x[2] == 0) break;
    while (*++x) {
      if (*x == 'q') { flagverbose = 0; continue; }
      if (*x == 'Q') { flagverbose = 1; continue; }
      if (*x == 'v') { if (flagverbose == 2) flagverbose = 3; else flagverbose = 2; continue; }
      if (*x == 'm') { flagmessage = 1; continue; }
      if (*x == 'M') { flagmessage = 0; continue; }
      if (*x == 'c') {
        if (x[1]) { keydir = x + 1; break; }
        if (argv[1]) { keydir = *++argv; break; }
      }
      die_usage(0);
    }
  }
  if (!nameparse(servername,*++argv)) die_usage("sname must be at most 255 bytes, at most 63 bytes between dots");
  if (!hexparse(serverlongtermpk,32,*++argv)) die_usage("pk must be exactly 64 hex characters");
  if (!multiipparse(serverip,sizeof serverip,*++argv)) die_usage("ip must be a comma-separated series of IPv4 addresses");
  if (!portparse(serverport,*++argv)) die_usage("port must be an integer between 0 and 65535");
  if (!hexparse(serverextension,16,*++argv)) die_usage("ext must be exactly 32 hex characters");
  if (!*++argv) die_usage("missing prog");

  for (;;) {
    r = open_read("/dev/null");
    if (r == -1) die_fatal("unable to open /dev/null",0,0);
    if (r > 9) { close(r); break; }
  }

  if (keydir) {
    fdwd = open_cwd();
    if (fdwd == -1) die_fatal("unable to open current working directory",0,0);
    if (chdir(keydir) == -1) die_fatal("unable to change to directory",keydir,0);
    if (load("publickey",clientlongtermpk,sizeof clientlongtermpk) == -1) die_fatal("unable to read public key from",keydir,0);
    if (load(".expertsonly/secretkey",clientlongtermsk,sizeof clientlongtermsk) == -1) die_fatal("unable to read secret key from",keydir,0);
  } else {
    crypto_box_keypair(clientlongtermpk,clientlongtermsk);
  }

  crypto_box_keypair(clientshorttermpk,clientshorttermsk);
  clientshorttermnonce = fastrandommod(281474976710656LL);
  crypto_box_beforenm(clientshortserverlong,serverlongtermpk,clientshorttermsk);
  crypto_box_beforenm(clientlongserverlong,serverlongtermpk,clientlongtermsk);

  udpfd4 = xsocket_udp(XSOCKET_V4);
  if (udpfd4 == -1) {
      debug_3(flagverbose + 1, "netcurvecpclient: warning: UDP4 socket: failed: ", e_str(errno), "\n");
  }
  else {
      debug_1(flagverbose, "netcurvecpclient: debug: UDP4 socket: created\n");
  }
  udpfd6 = xsocket_udp(XSOCKET_V6);
  if (udpfd6 == -1) {
      debug_3(flagverbose + 1, "netcurvecpclient: warning: UDP6 socket: failed: ", e_str(errno), "\n");
  }
  else {
      debug_1(flagverbose, "netcurvecpclient: debug: UDP6 socket: created\n");
  }
  if (udpfd4 == -1 && udpfd6 == -1) die_fatal("unable to create socket",0,0);

  for (hellopackets = 0;hellopackets < NUMIP;++hellopackets) {
    recent = nanoseconds();

    /* send a Hello packet: */

    clientextension_init();

    clientshorttermnonce_update();
    byte_copy(nonce,16,"CurveCP-client-H");
    uint64_pack(nonce + 16,clientshorttermnonce);

    byte_copy(packet,8,"QvnQ5XlH");
    byte_copy(packet + 8,16,serverextension);
    byte_copy(packet + 24,16,clientextension);
    byte_copy(packet + 40,32,clientshorttermpk);
    byte_copy(packet + 72,64,allzero);
    byte_copy(packet + 136,8,nonce + 16);
    crypto_box_afternm(text,allzero,96,nonce,clientshortserverlong);
    byte_copy(packet + 144,80,text + 16);

    udpfdtype = xsocket_type(serverip + 16 * hellopackets);
    if (udpfdtype == XSOCKET_V4) udpfd = udpfd4;
    if (udpfdtype == XSOCKET_V6) udpfd = udpfd6;
    r = xsocket_send(udpfd,udpfdtype,packet,224,serverip + 16 * hellopackets,serverport,0);
    if (r == -1) {
        debug_5(flagverbose + 1, "netcurvecpclient: warning: CurveCP hello packet: failed to: ", iptostr(0, serverip + 16 * hellopackets), ": ", e_str(errno), "\n");
        continue;
    }
    else {
        debug_3(flagverbose, "netcurvecpclient: debug: CurveCP hello packet: sent to: ", iptostr(0, serverip + 16 * hellopackets), "\n");
    }

    nextaction = recent + hellowait[hellopackets] + fastrandommod(hellowait[hellopackets]);

    for (;;) {
      long long timeout = nextaction - recent;
      long long plen = 0;
      if (timeout <= 0) break;
      if (udpfd4 != -1) {
        p[0].fd = udpfd4;
        p[0].events = POLLIN;
        p[0].revents = 0;
        ++plen;
      }
      if (udpfd6 != -1) {
        p[1].fd = udpfd6;
        p[1].events = POLLIN;
        p[1].revents = 0;
        ++plen;
      }
      debug_3(flagverbose, "netcurvecpclient: debug: waiting: max ", numtostr(0, timeout / 1000000 + 1), " milliseconds\n");

      if (poll(p,plen,timeout / 1000000 + 1) < 0) {
          p[0].revents = 0;
          p[1].revents = 0;
      }

      if (p[1].revents) {
        if (p[1].fd == udpfd4) udpfdtype = XSOCKET_V4;
        if (p[1].fd == udpfd6) udpfdtype = XSOCKET_V6;
        p[0].revents = 0;
      }

      if (p[0].revents) {
        if (p[0].fd == udpfd4) udpfdtype = XSOCKET_V4;
        if (p[0].fd == udpfd6) udpfdtype = XSOCKET_V6;
        p[1].revents = 0;
      }

      do { /* try receiving a Cookie packet: */
        if (!p[1].revents && !p[0].revents) break;
        r = xsocket_recv(udpfd,udpfdtype,packet,sizeof packet,packetip,packetport,0);
        if (r != 200) break;
        if (!(byte_isequal(packetip,16,serverip + 16 * hellopackets) &
              byte_isequal(packetport,2,serverport) &
              byte_isequal(packet,8,"RL3aNMXK") &
              byte_isequal(packet + 8,16,clientextension) &
              byte_isequal(packet + 24,16,serverextension)
           )) break;
        byte_copy(nonce,8,"CurveCPK");
        byte_copy(nonce + 8,16,packet + 40);
        byte_zero(text,16);
        byte_copy(text + 16,144,packet + 56);
        if (crypto_box_open_afternm(text,text,160,nonce,clientshortserverlong)) break;
        byte_copy(servershorttermpk,32,text + 32);
        byte_copy(servercookie,96,text + 64);
        byte_copy(serverip,16,serverip + 16 * hellopackets);
        debug_3(flagverbose, "netcurvecpclient: debug: CurveCP cookie packet: received from: ", iptostr(0, serverip + 16 * hellopackets), "\n");
        goto receivedcookie;
      } while (0);

      recent = nanoseconds();
    }
  }

  errno = ETIMEDOUT; die_fatal("no response from server",0,0);

  receivedcookie:

  crypto_box_beforenm(clientshortservershort,servershorttermpk,clientshorttermsk);

  byte_copy(nonce,8,"CurveCPV");
  if (keydir) {
    if (safenonce(nonce + 8,0) == -1) die_fatal("nonce-generation disaster",0,0);
  } else {
    fastrandombytes(nonce + 8,16);
  }

  byte_zero(text,32);
  byte_copy(text + 32,32,clientshorttermpk);
  crypto_box_afternm(text,text,64,nonce,clientlongserverlong);
  byte_copy(vouch,16,nonce + 8);
  byte_copy(vouch + 16,48,text + 16);

  /* server is responding, so start child: */

  writemessage(0); /* XXX */

  if (open_pipe(tochild) == -1) die_fatal("unable to create pipe",0,0);
  if (open_pipe(fromchild) == -1) die_fatal("unable to create pipe",0,0);
  
  child = fork();
  if (child == -1) die_fatal("unable to fork",0,0);
  if (child == 0) {
    if (keydir) if (fchdir(fdwd) == -1) die_fatal("unable to chdir to original directory",0,0);
    close(8);
    if (dup(tochild[0]) != 8) die_fatal("unable to dup",0,0);
    close(9);
    if (dup(fromchild[1]) != 9) die_fatal("unable to dup",0,0);
    /* XXX: set up environment variables */
    signal(SIGPIPE,SIG_DFL);
    debug_exec(flagverbose, "netcurvecpclient: debug: ", "executing: ", argv);
    execvp(*argv,argv);
    die_fatal("unable to run",*argv,0);
  }

  close(fromchild[1]);
  close(tochild[0]);


  for (;;) {
    p[0].fd = udpfd;
    p[0].events = POLLIN;
    p[1].fd = fromchild[0];
    p[1].events = POLLIN;

    if (poll(p,2,-1) < 0) {
      p[0].revents = 0;
      p[1].revents = 0;
    }

    do { /* try receiving a Message packet: */
      if (!p[0].revents) break;
      r = xsocket_recv(udpfd,udpfdtype,packet,sizeof packet,packetip,packetport,0);
      if (r < 80) break;
      if (r > 1152) break;
      if (r & 15) break;
      packetnonce = uint64_unpack(packet + 40);
      if (flagreceivedmessage && packetnonce <= receivednonce) break;
      if (!(byte_isequal(packetip,16,serverip + 16 * hellopackets) &
            byte_isequal(packetport,2,serverport) &
            byte_isequal(packet,8,"RL3aNMXM") &
            byte_isequal(packet + 8,16,clientextension) &
            byte_isequal(packet + 24,16,serverextension)
         )) break;
      byte_copy(nonce,16,"CurveCP-server-M");
      byte_copy(nonce + 16,8,packet + 40);
      byte_zero(text,16);
      byte_copy(text + 16,r - 48,packet + 48);
      if (crypto_box_open_afternm(text,text,r - 32,nonce,clientshortservershort)) break;

      if (!flagreceivedmessage) {
        flagreceivedmessage = 1;
	fastrandombytes(clientlongtermpk,sizeof clientlongtermpk);
	fastrandombytes(vouch,sizeof vouch);
	fastrandombytes(servername,sizeof servername);
	fastrandombytes(servercookie,sizeof servercookie);
      }

      receivednonce = packetnonce;
      text[31] = (r - 64) >> 4;
      /* child is responsible for reading all data immediately, so we won't block: */
      if (writeall(tochild[1],text + 31,r - 63) == -1) goto done;
    } while (0);

    do { /* try receiving data from child: */
      long long i;
      if (!p[1].revents) break;
      r = read(fromchild[0],childbuf,sizeof childbuf);
      if (r == -1) if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) break;
      if (r <= 0) goto done;
      childbuflen = r;
      for (i = 0;i < childbuflen;++i) {
	if (childmessagelen < 0) goto done;
	if (childmessagelen >= sizeof childmessage) goto done;
        childmessage[childmessagelen++] = childbuf[i];
	if (childmessage[0] & 128) goto done;
	if (childmessagelen == 1 + 16 * (unsigned long long) childmessage[0]) {
	  clientextension_init();
	  clientshorttermnonce_update();
          uint64_pack(nonce + 16,clientshorttermnonce);
	  if (flagreceivedmessage) {
	    r = childmessagelen - 1;
	    if (r < 16) goto done;
	    if (r > 1088) goto done;
            byte_copy(nonce,16,"CurveCP-client-M");
	    byte_zero(text,32);
	    byte_copy(text + 32,r,childmessage + 1);
	    crypto_box_afternm(text,text,r + 32,nonce,clientshortservershort);
	    byte_copy(packet,8,"QvnQ5XlM");
	    byte_copy(packet + 8,16,serverextension);
	    byte_copy(packet + 24,16,clientextension);
	    byte_copy(packet + 40,32,clientshorttermpk);
	    byte_copy(packet + 72,8,nonce + 16);
	    byte_copy(packet + 80,r + 16,text + 16);
            xsocket_send(udpfd,udpfdtype,packet,r + 96,serverip,serverport,0);
	  } else {
	    r = childmessagelen - 1;
	    if (r < 16) goto done;
	    if (r > 640) goto done;
	    byte_copy(nonce,16,"CurveCP-client-I");
	    byte_zero(text,32);
	    byte_copy(text + 32,32,clientlongtermpk);
	    byte_copy(text + 64,64,vouch);
	    byte_copy(text + 128,256,servername);
	    byte_copy(text + 384,r,childmessage + 1);
	    crypto_box_afternm(text,text,r + 384,nonce,clientshortservershort);
	    byte_copy(packet,8,"QvnQ5XlI");
	    byte_copy(packet + 8,16,serverextension);
	    byte_copy(packet + 24,16,clientextension);
	    byte_copy(packet + 40,32,clientshorttermpk);
	    byte_copy(packet + 72,96,servercookie);
	    byte_copy(packet + 168,8,nonce + 16);
	    byte_copy(packet + 176,r + 368,text + 16);
            xsocket_send(udpfd,udpfdtype,packet,r + 544,serverip,serverport,0);
	  }
	  childmessagelen = 0;
	}
      }
    } while (0);
  }


  done:

  do {
    r = waitpid(child,&childstatus,0);
  } while (r == -1 && errno == EINTR);

  if (!WIFEXITED(childstatus)) { errno = 0; die_fatal("process killed by signal",0,0); }
  return WEXITSTATUS(childstatus);
}
