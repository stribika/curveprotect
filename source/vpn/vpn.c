/*
 * 20110906
 * Jan Mojzis
 * Public domain.
 */

#include <unistd.h>
#include <signal.h>
#include "sig.h"
#include "ncommands.h"
#include "stralloc.h"
#include "strerr.h"
#include "buffer.h"
#include "byte.h"
#include "tun.h"
#include "fd.h"
#include "droproot.h"
#include "timeoutread.h"
#include "timeoutwrite.h"
#include "fmt.h"
#include "str.h"
#include "env.h"
#include "sgetopt.h"
#include "cdb.h"
#include "hexparse.h"
#include "open.h"
#include "scan.h"
#include "openreadclose.h"
#include "lock.h"
#include "writeall.h"
#include "deepsleep.h"
#include "error.h"
#include "wait.h"
#include "ip4.h"
#include "fdcp.h"
#include "oknet.h"


#define FATAL "vpn: fatal: "
#define WARNING "vpn: warning: "
#define INFO "vpn: info: "

#define USAGE "\n\
vpn: how to use:\n\
vpn:   -s (optional): VPN is in server mode\n\
vpn:   -c (optional): VPN is in client mode (default)\n\
vpn:   -h (optional): help\n\
"

static void netputs(const char *s);
static void netflush(void);

int flagserver = 0;
void ie(void){
    if (!flagserver) return;
    netputs("eInternal Error");netflush();
    close(0);
    close(1);
}

static void usage(void){ie();strerr_die1x(100,USAGE);}
static void die_nomem(void){ie();strerr_die2x(111,FATAL,"out of memory");}
static void die_fork(void){ie();strerr_die2sys(111,FATAL,"unable to fork: ");}
static void die_handshake(void){ie();strerr_die2x(111,FATAL,"unable to parse data from remote server: protocol error");}
static void die_read(void){ie();strerr_die2sys(111,FATAL,"unable to read from pipe: ");}
static void die_readfn(const char *s){ie();strerr_die4sys(111,FATAL,"unable to read file ",s,": ");}
static void die_openfn(const char *s){ie();strerr_die4sys(111,FATAL,"unable to open file ",s,": ");}
static void die_writefn(const char *s){ie();strerr_die4sys(111,FATAL,"unable to write to file ",s,": ");}
static void die_lockfn(const char *s){ie();strerr_die4sys(111,FATAL,"unable to lock file ",s,": ");}
static void die_unlockfn(const char *s){ie();strerr_die4sys(111,FATAL,"unable to unlock file ",s,": ");}
static void die_write(void){ie();strerr_die2sys(111,FATAL,"unable to write to pipe: ");}
static void die_tuninit(void){ie();strerr_warn2(FATAL,"unable to init tun device: ",&strerr_sys);deepsleep(60);_exit(111);}
static void die_tunip(void){ie();strerr_die2sys(111,FATAL,"unable to assign IP on tun device: ");}
static void die_tunroute(void){ie();strerr_die2sys(111,FATAL,"unable to assign route on tun device: ");}
static void die_fdcopy(const char *s){ie();strerr_die4sys(111,FATAL,"unable to copy fd to ",s,": ");;}
static void die_fdmove(const char *s){ie();strerr_die4sys(111,FATAL,"unable to move fd to ",s,": ");;}
static void die_cdbread(void){ie();strerr_die2sys(111,FATAL,"unable to read data.cdb: ");}
static void die_cdbformat(void){ie();strerr_die3x(111,FATAL,"unable to read data.cdb: ","format error");}
static void die_exist(const char *s){ie();strerr_die4x(111,FATAL,"unable start vpn session for public-key ",s,": exist");}
static void die_perm(const char *s){ie();strerr_die4x(111,FATAL,"unable to kill process ",s,": ");}
static void die_parsepk(void){ie();strerr_die2x(111,FATAL,"unable to parse $REMOTEPK");}
static void die_pknotset(void){ie();strerr_die2x(111,FATAL,"$REMOTEPK not set");}
static void die_servernotset(void){ie();strerr_die2x(111,FATAL,"$SERVER not set");}
static void die_portnotset(void){ie();strerr_die2x(111,FATAL,"$PORT not set");}

static const char *server = 0;
static const char *port = 0;
static const char *pkstr = 0;

static void finished(void){strerr_die2x(0,INFO,"finished");}
static void starting(void){strerr_warn2(INFO,"starting",0);}
static void connected(void){strerr_warn5(INFO,"connected to ",server,":",port,0);}
static void sconnected(void){strerr_warn3(INFO,"access: ",pkstr,0);}

static int safewrite(int fd, char *buf, unsigned int len) {
    int r;
    r = timeoutwrite(60, fd, buf, len);
    if (r <= 0) die_write();
    return r;
}
static char bwspace[1024];
static buffer bw;


static int saferead(int fd, char *buf, unsigned int len) {
    int r;
    r = timeoutread(60, fd, buf, len);
    if (r <= 0) die_read();
    return r;
}
static char brspace[1024];
static buffer br;

static char strnum[FMT_ULONG];
static void netput(const char *s, unsigned int len) {
    buffer_put(&bw,strnum,fmt_ulong(strnum,len));
    buffer_puts(&bw,":");
    buffer_put(&bw,s,len);
    buffer_puts(&bw,",");
    return;
}
static void netputs(const char *s) {
    netput(s, str_len(s));
}

static void netflush(void) {
    buffer_flush(&bw);
}

int vpn_kill(int p, int s) {

    int pid;
    int status;
    int r;

    pid = fork();
    if (pid == -1) die_fork();

    if (pid == 0){
        droproot(FATAL);
        r = kill(p, s);
        if (r == -1) _exit(errno);
        _exit(0);
    }

    do {
        r = wait_pid(&status,pid);
    } while (r == -1 && errno == error_intr);
    if (r == -1) return error_perm;
    if (wait_crashed(status)) return error_perm;
    return wait_exitcode(status);
}

stralloc lockfn = {0};
stralloc lockpid = {0};
static void vpn_serverlock(const char *fn) {

    int r, k, s;
    int fd;
    unsigned int j;
    unsigned long pid;

    if (!stralloc_copys(&lockfn, "./lock/")) die_nomem();
    if (!stralloc_cats(&lockfn, fn)) die_nomem();
    if (!stralloc_0(&lockfn)) die_nomem();

    s = SIGTERM;
    for(j = 0; j < 20; ++j) {

        r = openreadclose(lockfn.s, &lockpid, 32);
        if (r == -1) die_readfn(lockfn.s);

        if (r){
            if (!stralloc_0(&lockpid)) die_nomem();
            if (!scan_ulong(lockpid.s, &pid)) continue;
            if (j > 15) s = SIGKILL;
            k = vpn_kill(pid, s);
            if (k > 0){
                if (k == error_perm) die_perm(lockpid.s);
                unlink(lockfn.s);
                continue;
            }
            deepsleep(1);
            continue;
        }

        fd = open_trunc(lockfn.s);
        if (fd == -1) die_openfn(lockfn.s);
        if (lock_ex(fd) == -1) die_lockfn(lockfn.s);
        if (!stralloc_copys(&lockpid,"")) die_nomem();
        if (!stralloc_catint(&lockpid,getpid())) die_nomem();
        if (writeall(fd, lockpid.s, lockpid.len) == -1) die_writefn(lockfn.s);
        if (fsync(fd) == -1) die_writefn(lockfn.s);
        if (lock_un(fd) == -1) die_unlockfn(lockfn.s);
        if (close(fd) == -1) die_writefn(lockfn.s);
        return;
    }
    die_exist(fn);
}

static int safeput(char *buf, unsigned int len) {
    char ch;
    unsigned int i;

    for (i = 0; i < len; ++i){
        ch = buf[i];
        if (ch == ' ') ch = '_';
        if (ch < 33) ch = '?';
        if (ch > 126) ch = '?';
        if (buffer_put(buffer_2, &ch, 1) == -1) return -1;
    }
    return 0;
}

static void vpn_ok(char *arg, unsigned int len) {
    if (!flagserver){
        buffer_puts(buffer_2, INFO);
        buffer_puts(buffer_2, "server okcmd: ");
        safeput(arg, len);
        buffer_puts(buffer_2, "\n");
        buffer_flush(buffer_2);
    }
    return;
}
static void vpn_notimplemented(char *arg, unsigned int len) {return;}

static char ipstr[IP4_FMT];
static char gwstr[IP4_FMT];

static int seenip = 0;
static char ipgw[8];
static void vpn_ip4(char *arg, unsigned int len) {

    int ret = 0;

    if (seenip) return;

    if (len != 8) die_handshake();
    ipstr[ip4_fmt(ipstr,arg + 0)] = 0;
    gwstr[ip4_fmt(gwstr,arg + 4)] = 0;
    ret = oknet("ip", arg, "\377\377\377\377");
    byte_copy(ipgw, 8, arg);
    seenip = 1;

    if (!flagserver) {
        buffer_puts(buffer_2, INFO);
        buffer_puts(buffer_2, "server ip4cmd: ");
        buffer_puts(buffer_2, ipstr);
        buffer_puts(buffer_2, " ");
        buffer_puts(buffer_2, gwstr);
        switch (ret) {
            case 0:
                buffer_puts(buffer_2, " ... rejected due configuration, exiting ...\n");
                buffer_flush(buffer_2);
                deepsleep(60);
                _exit(111);
                break;
            case 1:
                buffer_puts(buffer_2, " ... accepted\n");
                buffer_flush(buffer_2);
                break;
            default:
                buffer_puts(buffer_2, " ... rejected, unknown error, exiting ...\n");
                buffer_flush(buffer_2);
                deepsleep(60);
                _exit(111);
                break;
        }
    }
    return;
}

static stralloc routes = {0};
static void vpn_route4(char *arg, unsigned int len) {

    int ret = 0;

    if (len != 8) die_handshake();
    ipstr[ip4_fmt(ipstr,arg + 0)] = 0;
    gwstr[ip4_fmt(gwstr,arg + 4)] = 0;
    ret = oknet("route", arg, arg + 4);
    if (ret) if (!stralloc_catb(&routes,arg,len)) die_nomem();
    if (!flagserver) {
        buffer_puts(buffer_2, INFO);
        buffer_puts(buffer_2, "server route4cmd: ");
        buffer_puts(buffer_2, ipstr);
        buffer_puts(buffer_2, "/");
        buffer_puts(buffer_2, gwstr);
        if (ret) {
            buffer_puts(buffer_2, " ... accepted");
        }
        else {
            buffer_puts(buffer_2, " ... rejected due configuration");
        }
        buffer_puts(buffer_2, "\n");
        buffer_flush(buffer_2);
    }
    return;
}

static void vpn_error(char *arg, unsigned int len) {

    if (!flagserver) {
        buffer_puts(buffer_2, FATAL);
        buffer_puts(buffer_2, "server errorcmd: ");
        safeput(arg, len);
        buffer_puts(buffer_2, "\n");
        buffer_flush(buffer_2);
    }
    deepsleep(60); 
    _exit(111);
}

static int flagverbose = 0;

static stralloc tunname = {0};
static void vpn_client_doit(char *arg, unsigned int len) {

    int fd;
    unsigned int i;

    if (!seenip) die_handshake();

    fd = tun_init(&tunname);
    if (fd == -1) die_tuninit();
    if (tun_ip4(&tunname, ipgw, ipgw + 4) == -1) die_tunip();

    for (i = 0;i + 8 <= routes.len;i += 8){
        if (tun_route4(&tunname, routes.s + i, routes.s + i + 4, ipgw + 4) == -1)
            die_tunroute();
    }

    droproot(FATAL);

    if (fd_copy(1,fd) == -1) die_fdcopy("1");
    if (fd_move(0,fd) == -1) die_fdmove("0");

    netputs("0go");
    netflush();
    connected();
    if (fdcp_vpn(FATAL,0,7,86400,6,1,120, flagverbose, flagserver) == -1) _exit(111);
    finished();
}

static void vpn_server_doit(char *arg, unsigned int len) {

    int fd;

    vpn_serverlock(pkstr);

    fd = tun_init(&tunname);
    if (fd == -1) die_tuninit();
    if (tun_ip4(&tunname, ipgw + 4, ipgw) == -1) die_tunip();

    droproot(FATAL);

    if (fd_copy(6,fd) == -1) die_fdcopy("6");
    if (fd_move(7,fd) == -1) die_fdmove("7");
    sconnected();
    /* net, local */
    if (fdcp_vpn(FATAL,0,7,1800,6,1,86400, flagverbose, flagserver) == -1) _exit(111);
    finished();
}

/* XXX TODO:
I, R, d, D commands
I .. ipv6
R .. ipv6 route
d .. ipv4 DNS record
D .. ipv6 DNS record
*/

static struct ncommands vpnclient[] = {
  { "o", vpn_ok, 0 }
, { "e", vpn_error, 0 }
, { "i", vpn_ip4, 0 }
, { "r", vpn_route4, 0 }
, { "0", vpn_client_doit, 0 }
, { 0, vpn_notimplemented, 0 }
} ;

static struct ncommands vpnserver[] = {
  { "o", vpn_ok, 0 }
, { "e", vpn_error, 0 }
, { "0", vpn_server_doit, 0 }
, { 0, vpn_notimplemented, 0 }
} ;

static struct cdb c;
static int fd;

uint32 dlen;

unsigned char pk[32];
char data[1024];
int flagallowed = 0;


char *keybuf = 0;


void signalhandler(int s) {
    _exit(0);
}


int main(int argc, char **argv, char **envp) {

    char *x;
    int opt;
    int r;

    x = env_get("ROOT");
    if (!x)
        strerr_die2x(111,FATAL,"$ROOT not set");
    if (chdir(x) == -1)
        strerr_die4sys(111,FATAL,"unable to chdir to ",x,": ");

    sig_ignore(sig_pipe);
    sig_catch(sig_term, signalhandler);

    while ((opt = getopt(argc,argv,"cshv")) != opteof) {
        switch(opt) {
            case 's':  flagserver = 1; break;
            case 'c':  flagserver = 0; break;
            case 'v':  flagverbose = 1; break;
            case 'h':  usage();
            default:   break;
        }
    }

    if (flagserver) {

        if (argc > 1)
            if (str_len(argv[argc - 1]) == (2*sizeof pk))
                keybuf = argv[argc - 1];

        buffer_init(&br, saferead,  0, brspace, sizeof brspace);
        buffer_init(&bw, safewrite, 1, bwspace, sizeof bwspace);

        x = env_get("REMOTEPK");
        if (!x) die_pknotset();
        if (!hexparse(pk, sizeof pk, x)) die_parsepk();
        pkstr = x;

        if (keybuf) if (pkstr) byte_copy(keybuf,2*sizeof pk, pkstr);

        fd = open_read("data.cdb");
        if (fd == -1) die_cdbread();
        cdb_init(&c,fd);

        /* is desired ? */
        cdb_findstart(&c);
        for (;;) {
            r = cdb_findnext(&c,(char *)pk,sizeof pk);
            if (r == -1) die_cdbread();
            if (!r) break;
            dlen = cdb_datalen(&c);
            if (dlen > sizeof data) die_cdbformat();
            if (cdb_read(&c,data,dlen,cdb_datapos(&c)) == -1) die_cdbformat();

            switch(data[0]) {
                case 'i':
                    flagallowed = 1;
                    break;
                default:
                    break;
                    
            }
        }


        if (!flagallowed) {
            netputs("eAccess Denied"); netflush();
            strerr_die4x(111,FATAL,"publickey ",pkstr," not desired");
        }
        netputs("oAccess Allowed");

        /* users setings */
        cdb_findstart(&c);
        for (;;) {
            r = cdb_findnext(&c,(char *)pk,sizeof pk);
            if (r == -1) die_cdbread();
            if (!r) break;
            dlen = cdb_datalen(&c);
            if (dlen > sizeof data) die_cdbformat();
            if (cdb_read(&c,data,dlen,cdb_datapos(&c)) == -1) die_cdbformat();

            switch(data[0]) {
                case 'i':
                    if (dlen != 9) die_cdbformat();
                    if (!seenip) {
                        byte_copy(ipgw, 8, data + 1);
                        netput(data,dlen);
                        seenip = 1;
                    }
                    break;
                case 'r':
                    if (dlen != 9) die_cdbformat();
                    netput(data,dlen);
                    break;
                default:
                    break;

            }
        }

        /* global setings */
        cdb_findstart(&c);
        for (;;) {
            r = cdb_findnext(&c,"",0);
            if (r == -1) die_cdbread();
            if (!r) break;
            dlen = cdb_datalen(&c);
            if (dlen > sizeof data) die_cdbformat();
            if (cdb_read(&c,data,dlen,cdb_datapos(&c)) == -1) die_cdbformat();

            switch(data[0]) {
                case 'r':
                    if (dlen != 9) die_cdbformat();
                    netput(data,dlen);
                    break;
                default:
                    break;
            }
        }

        cdb_free(&c);
        close(fd);
        netputs("0go");
        netflush();

        ncommands(&br, vpnserver);
    }
    else {
        server = env_get("SERVER");
        if (!server) die_servernotset();
        port = env_get("PORT");
        if (!port) die_portnotset();
        starting();
        buffer_init(&br, saferead,  6, brspace, sizeof brspace);
        buffer_init(&bw, safewrite, 7, bwspace, sizeof bwspace);
        ncommands(&br, vpnclient);
    }
    return 111;
}
