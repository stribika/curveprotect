/*
 * 20110504
 * Jan Mojzis
 * Public domain.
 */

#include "strerr.h"
#include "buffer.h"
#include "fmt.h"
#include "stralloc.h"
#include "error.h"
#include "case.h"
#include "timeoutwrite.h"
#include "timeoutread.h"
#include "getln.h"
#include "scan.h"
#include "sig.h"
#include "str.h"
#include "byte.h"
#include "pathexec.h"
#include "ip4.h"
#include <signal.h>
#include <unistd.h> /* _exit, pipe, fork, close */
#include <sys/socket.h> /* shutdown */
#include "ndelay.h"
#include "fd.h"
#include "http.h"
#include "deepsleep.h"
#include "httpdate.h"
#include "env.h"
#include "fdcp.h"
#include "wait.h"
#include "uint64.h"
#include "iptostr.h"

const char *fatal = "httpproxy: fatal: ";
const char *info = "httpproxy: info: ";
const char *starting = "";

int flagcurvecp = 0;

stralloc header  = {0};

char strport[FMT_ULONG];
char strnum[FMT_ULONG];

void die_clientwrite(void){strerr_die2sys(111,fatal,"unable to write to client: ");}
void die_clientread(void){strerr_die2sys(111,fatal,"unable to read from client: ");}

static void die_serverwrite(void);
static void die_serverread(void);
static void die_nomem(void);

char clientoutbuf[1024];
char clientinbuf[1024];
char serveroutbuf[1024];
char serverinbuf[1024];

int flagchunked;
int protocolnum;
int flagrequestbody;
long long cl = -1;
unsigned long long ull;


int serversafewrite(int fd, char *buf, int len){

    int r;
    r = timeoutwrite(60, fd, buf, len);
    if (r <= 0) die_clientwrite();
    return r;
}

buffer clientout;

void clientout_put(char *s, int len){buffer_put(&clientout, s, len);}
void clientout_puts(const char *s){buffer_puts(&clientout, s);}
void clientout_flush(void){buffer_flush(&clientout);}

buffer serverout = BUFFER_INIT(serversafewrite, 1, serveroutbuf, sizeof serveroutbuf);

void serverout_put(char *s, int len){
    buffer_put(&serverout, s, len);
}
void serverout_puts(const char *s){
    buffer_puts(&serverout, s);
}
void serverout_putnum(unsigned long num){serverout_put(strnum,fmt_ulong(strnum,num));}
void serverout_flush(void){buffer_flush(&serverout);}

int clientsaferead(int fd, char *buf, int len){

    int r;
    clientout_flush();
    r = timeoutread(60,fd,buf,len);
    if (r == 0) _exit(0);
    /* if (r == 0) die_clientread(); */
    return r;
}

buffer clientin = BUFFER_INIT(clientsaferead, 0, clientinbuf, sizeof clientinbuf);
int clientin_get(char *s, int len){return buffer_get(&clientin, s, len);}

unsigned long long serverbytes = 0;

int serversaferead(int fd, char *buf, int len){

    int r;
    serverout_flush();
    r = buffer_unixread(fd,buf,len);
    if (r == -1) die_serverread();
    serverbytes += r;
    return r;
}

buffer serverin;
int serverin_get(char *s, int len){
    int r;
    r = buffer_get(&serverin, s, len);
    if (r == 0) _exit(0);
    return r;
}



/* new */
stralloc line = {0};
stralloc host  = {0};
stralloc field = {0};


void serverreadline(void){

    int match;

    if (getln(&serverin,&line,&match,'\n') == -1) die_serverread();
    if (!match) die_serverread();
    if (line.len && (line.s[line.len - 1] == '\n')) --line.len;
    if (line.len && (line.s[line.len - 1] == '\r')) --line.len;
    line.s[line.len] = 0;
    return;
}

void clientreadline(void){

    int match;

    if (getln(&clientin,&line,&match,'\n') == -1) die_clientread();
    if (!match) die_clientread();
    if (line.len && (line.s[line.len - 1] == '\n')) --line.len;
    if (line.len && (line.s[line.len - 1] == '\r')) --line.len;
    line.s[line.len] = 0;
    return;
}

struct tai now;
stralloc nowstr={0};

void xheader(int code){


    if (protocolnum == 1)
        serverout_puts("HTTP/1.0 ");
    else
        serverout_puts("HTTP/1.1 ");
    serverout_putnum(code);
    serverout_puts(" ");
    serverout_puts(http_str(code));
    serverout_puts("\r\nServer: httpproxy/1.0\r\nDate:");
    tai_now(&now);
    if (!httpdate(&nowstr,&now)) die_nomem();
    serverout_put(nowstr.s,nowstr.len);
    serverout_puts("\r\n");
    return;
}

/* global */
int flagbody = 1;
int flagconnect = 0;



void die_5(int code
    ,const char *s0
    ,const char *s1
    ,const char *s2
    ,const char *s3
    ,const char *s4
){
    const char *s[5];
    unsigned int i;
    unsigned int len = 60;

    s[0] = s0;
    s[1] = s1;
    s[2] = s2;
    s[3] = s3;
    s[4] = s4;

    for(i = 0; s[i]; ++i){
        len += str_len(s[i]);
    }

    if (protocolnum > 0) {
        xheader(code);
        serverout_puts("Content-Length: ");
        serverout_putnum(len);
        serverout_puts("\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n");
    }
    if (flagbody) {
        serverout_puts("<html><body><h2>proxy error:</h2><pre>");
        for(i = 0; s[i]; ++i){
            serverout_puts(s[i]);
        }
        serverout_puts("</pre></body></html>\r\n");
    }

    http_log_fatal(&header, s);
    serverout_flush();
    if (protocolnum >= 2) {
        shutdown(1,1);
        deepsleep(1);
    }
    _exit(0);
}
#define die_4(x,a,b,c,d) die_5(x,a,b,c,d,0)
#define die_3(x,a,b,c) die_4(x,a,b,c,0)
#define die_2(x,a,b) die_3(x,a,b,0)
#define die_1(x,a) die_2(x,a,0)
#define die_0(x) die_1(x,0)

/* error definitions*/
void die_badproto(void){protocolnum = 0; die_1(505, "protocol 0.9 not supported");}
void die_badrequest(void){die_1(400, "bad request");}
void die_accessdenied(void){die_1(403, "unable to connect to localhost IP (httpproxy rule): access denied");}
void die_fork(void){die_2(503, "unable to fork: ",error_str(errno));}
void die_pipe(void){die_2(503,"unable to create pipe: ", error_str(errno));}
void die_fdmove(void){die_2(503,"unable to move filedescriptor: ", error_str(errno));}
void die_method(const char *s){die_3(501, "method ", s, " not implemented");}
void die_nomem(void){die_1(503,"out of memory");}
void die_notencrypted(void){die_1(503,"unencrypted connection not allowed");}
void die_child(void){die_2(503,"unable to connect: ", error_str(errno));}

static stralloc cmd = {0};
static void die_exec(char **s){

    int i;
    if (!stralloc_copys(&cmd,"unable to run: ")) die_nomem();
    for(i = 0; s[i]; ++i){
        if (!stralloc_cats(&cmd,s[i])) die_nomem();
        if (!stralloc_cats(&cmd," ")) die_nomem();
    }
    cmd.len -= 1;
    if (!stralloc_cats(&cmd,": ")) die_nomem();
    if (!stralloc_0(&cmd)) die_nomem();
    die_2(503, cmd.s, error_str(errno));
}

void die_serverwrite(void){die_2(503,"unable to write to server: ", error_str(errno));}
void die_serverread(void){die_2(503,"unable to read from server: ", error_str(errno));}

char buf[1024];
void servercopyall(unsigned long long size){

    int r;

    while(size > 0){
        if (size > sizeof buf)
            r = serverin_get(buf, sizeof buf);
        else
            r = serverin_get(buf, size);
        size -= r;
        serverout_put(buf, r);
    }
}

void clientcopyall(unsigned long long size){

    int r;

    while(size > 0){
        if (size > sizeof buf)
            r = clientin_get(buf, sizeof buf);
        else
            r = clientin_get(buf, size);
        size -= r;
        clientout_put(buf, r);
        if (!stralloc_catb(&header, buf, r)) die_nomem();
    }
}


void hostport(stralloc *ooo, stralloc *ppp, int *flagip, stralloc *iii) {

    unsigned int a,b,c;

    a = byte_chr(iii->s,iii->len,'[');
    b = byte_rchr(iii->s,iii->len,']');
    c = byte_rchr(iii->s,iii->len,':');

    *flagip = 0;

    if (a != iii->len && b != iii->len && b > a) {
        /* IP */
        if (!stralloc_copyb(ooo, iii->s + a + 1, b - a - 1)) die_nomem();
        if (c != iii->len && c > b) {
            if (!stralloc_copyb(ppp, iii->s + c + 1, c - b - 1)) die_nomem();
        }
        *flagip = 1;
    }
    else {
        if (!stralloc_copyb(ooo, iii->s, c)) die_nomem();
        if (c != iii->len) {
            if (!stralloc_copyb(ppp, iii->s + c + 1, iii->len - c - 1 )) die_nomem();
        }
    }
    return;
}


stralloc version = {0};
stralloc status = {0};
stralloc statusmessage = {0};

void response(void){

    unsigned int spaces,i;
    unsigned long ul;


    /* status line */
    for(;;){
        serverreadline();
        serverout_put(line.s, line.len);
        serverout_puts("\r\n");
        if(line.len) break;
    }

    if (!stralloc_copys(&version, "")) die_nomem();
    if (!stralloc_copys(&status, "")) die_nomem();
    if (!stralloc_copys(&statusmessage, "")) die_nomem();
    flagchunked = 0;
    cl = -1;
    flagrequestbody = 0;
    protocolnum = 2;

    spaces = 0;
    for (i = 0;i < line.len;++i){
        if (line.s[i] == ' ' && spaces < 2) {
            if (!i || (line.s[i - 1] != ' ')) {
                ++spaces;
            }
        }
        else{
            switch(spaces) {
                case 0:
                    if (!stralloc_append(&version,line.s + i)) die_nomem();
                    break;
                case 1:
                    if (!stralloc_append(&status,line.s + i)) die_nomem();
                    break;
                case 2:
                    if (!stralloc_append(&statusmessage,line.s + i)) die_nomem();
                break;
            }
        }
    }


    if (!stralloc_copys(&field,"")) die_nomem();
    for (;;) {
        serverreadline();
        if (!line.len || ((line.s[0] != ' ') && (line.s[0] != '\t'))){

            /* header Content-Length: */
            if (case_startb(field.s,field.len,"content-length: ")){
                status.s[status.len] = 0;
                if (str_diff(status.s,"304")){
                    field.s[field.len] = 0;
                    if (scan_ulonglong(field.s+16,&ull)){
                        cl = ull;
                    }
                }
            }
            /* header Transfer-Encoding: */
            if (case_startb(field.s,field.len,"transfer-encoding: chunked")){
                flagchunked = 1;
            }

            /* XXX - put header */
            if (!case_startb(field.s,field.len,"connection:")){
                if (!case_startb(field.s,field.len,"keep-alive:")){
                    if (!case_startb(field.s,field.len,"proxy-connection:")){
                        if (!case_startb(field.s,field.len,"x-curvecp:")){
                            if (field.len){
                                serverout_put(field.s, field.len);
                                serverout_puts("\r\n");
                            }
                        }
                    }
                }
            }

            field.len = 0;
        }
        if (!line.len) break;
        if (!stralloc_cat(&field,&line)) die_nomem();
    }
    if (flagcurvecp) {
        serverout_puts("X-CurveCP: true\r\n");
    }
    serverout_puts("Proxy-Connection: close\r\nConnection: close\r\n\r\n"); 

    if (flagbody){
        if (flagchunked){
            for(;;){
                serverreadline();
                serverout_put(line.s, line.len);
                serverout_puts("\r\n");
                if (!stralloc_0(&line)) die_nomem(); 
                i = scan_xlong(line.s,&ul);
                if (!i) {errno = error_proto; die_serverread();}
                if (ul == 0) break;
                servercopyall(ul);
                serverreadline();
                serverout_put(line.s, line.len);
                serverout_puts("\r\n");
                if (line.len) {errno = error_proto; die_serverread(); }
            }
            goto finish;
        }
        if (cl != -1){
            servercopyall(cl);
            goto finish;
        }

        /* XXX - expecting that server closes connection */
        switch(buffer_copy(&serverout, &serverin)) {
            case -2:
                die_serverread();
            case -3:
                die_serverwrite();
        }
    }
finish:
    serverout_flush();
}


stralloc method = {0};
stralloc protocol = {0};
stralloc url = {0};
stralloc logurl = {0};



stralloc out = {0};
int tochild[2] = {-1,-1};
int fromchild[2] = {-1,-1};
char *run[20];
char *ipstr = 0;

stralloc xhost = {0};
stralloc port = {0};
int flagip;

stralloc ips = {0};


char *keydir;

char messagebuf[32];
long long errnum;

int main(int argc, char **argv, char **envp){


    unsigned int spaces,i;
    unsigned long ul;
    int pid;
    int wstatus;
    int r = -1;

    sig_ignore(sig_pipe);

    keydir = env_get("KEYDIR");

    if (!stralloc_copys(&out,"")) die_nomem();

    for (;;) {
        /* status line */
        clientreadline();
        if (line.len) break;
    }


    if (!stralloc_copys(&method, "")) die_nomem();
    if (!stralloc_copys(&protocol, "")) die_nomem();
    if (!stralloc_copys(&url, "")) die_nomem();
    if (!stralloc_copys(&host, "")) die_nomem();
    if (!stralloc_copys(&header, "")) die_nomem();
    flagchunked = 0;
    cl = -1;
    flagrequestbody = 0;
    protocolnum = 2;
    flagbody = 1;
    flagconnect = 0;

    spaces = 0;
    for (i = 0;i < line.len;++i){
        if (line.s[i] == ' ') {
            if (!i || (line.s[i - 1] != ' ')) {
                ++spaces;
                if (spaces >= 3) break;
            }
        }
        else{
            switch(spaces) {
                case 0:
                    if (!stralloc_append(&method,line.s + i)) die_nomem();
                    break;
                case 1:
                    if (!stralloc_append(&url,line.s + i)) die_nomem();
                    break;
                case 2:
                    if (!stralloc_append(&protocol,line.s + i)) die_nomem();
                    break;
            }
        }
    }
    method.s[method.len] = 0;
    protocol.s[protocol.len]=0;
    url.s[url.len]=0;

    if (!stralloc_copy(&header, &method)) die_nomem();
    if (!stralloc_cats(&header, " ")) die_nomem();

    if (!protocol.len){
        if (!stralloc_cat(&header, &url)) die_nomem();
        if (!stralloc_cats(&header, "\n")) die_nomem();
        die_badproto();
    }
    if (case_equals(protocol.s,"http/1.0")){
        protocolnum = 1;
    }

    if (case_startb(url.s,url.len,"http://")) {
        if (!stralloc_copyb(&host,url.s + 7,url.len - 7)) die_nomem();
        i = byte_chr(host.s,host.len,'/');
        if (!stralloc_catb(&header, host.s + i, host.len - i)) die_nomem();
        if (!stralloc_catb(&logurl, host.s + i, host.len - i)) die_nomem();
        host.len = i;
    }
    else{
        if (!stralloc_cat(&header, &url)) die_nomem();
        if (!stralloc_cat(&logurl, &url)) die_nomem();
    }
    /* XXX - force HTTP/1.0 */
    if (!stralloc_cats(&header, " HTTP/1.0\r\n")) die_nomem();
    if (host.len){
        if (!stralloc_cats(&header, "Host: ")) die_nomem();
        if (!stralloc_cat(&header, &host)) die_nomem();
        if (!stralloc_cats(&header, "\r\n")) die_nomem();
    }

    /* headers */
    if (!stralloc_copys(&field,"")) die_nomem();
    for (;;) {
        clientreadline();
        if (!line.len || ((line.s[0] != ' ') && (line.s[0] != '\t'))) {


            /* header Content-Length: */
            if (case_startb(field.s,field.len,"content-length: ")){
                field.s[field.len] = 0;
                if (scan_ulonglong(field.s+16,&ull)){
                    cl = ull;
                }
            }
            /* header Host: */
            if (case_startb(field.s,field.len,"host:"))
                if (!host.len){
                    for (i = 5;i < field.len;++i)
                        if (field.s[i] != ' ')
                            if (field.s[i] != '\t')
                                if (!stralloc_append(&host,&field.s[i])) die_nomem();

                    if (!stralloc_cats(&header, "Host: ")) die_nomem();
                    if (!stralloc_cat(&header, &host)) die_nomem();
                    if (!stralloc_cats(&header, "\r\n")) die_nomem();
                }


            /* header Transfer-Encoding: */
            if (case_startb(field.s,field.len,"transfer-encoding: chunked")){
                flagchunked = 1;
            }

            /* put header */
            if (!case_startb(field.s,field.len,"connection:")){
                if (!case_startb(field.s,field.len,"keep-alive:")){
                    if (!case_startb(field.s,field.len,"proxy-connection:")){
                        if (!case_startb(field.s,field.len,"host:")){
                            if (field.len){
                                if (!stralloc_cat(&header, &field)) die_nomem();
                                if (!stralloc_cats(&header, "\r\n")) die_nomem();
                            }
                        }
                    }
                }
            }

            field.len = 0;
        }
        if (!line.len) break;
        if (!stralloc_cat(&field,&line)) die_nomem();
    }
    /* XXX - force Connection: close */
    if (!stralloc_cats(&header, "Connection: close\r\n\r\n")) die_nomem();

    /* check method */
    if (str_equal(method.s,"POST")) flagrequestbody = 1;
    else if (str_equal(method.s,"CONNECT")) flagconnect = 1;
    else if (str_equal(method.s,"HEAD")) flagbody = 0;
    else if (str_equal(method.s,"OPTIONS")) ;
    else if (!str_equal(method.s,"GET")) die_method(method.s);

    if (flagconnect) {
        hostport(&xhost, &port, &flagip, &url);
    }
    else {
        hostport(&xhost, &port, &flagip, &host);
        if (!port.len)
            if (!stralloc_copys(&port,"80")) die_nomem();
    }
    if (!stralloc_copy(&host,&xhost)) die_nomem();

    if (!host.len) die_badrequest();
    if (!port.len) die_badrequest();
    if (!stralloc_0(&host)) die_nomem(); --(host.len);
    if (!stralloc_0(&port)) die_nomem(); --(port.len);

    if (pipe(tochild) == -1) die_pipe();
    if (pipe(fromchild) == -1) die_pipe();
    if (ndelay_off(tochild[0]) == -1) die_pipe();
    if (ndelay_off(tochild[1]) == -1) die_pipe();
    if (ndelay_off(fromchild[0]) == -1) die_pipe();
    if (ndelay_off(fromchild[1]) == -1) die_pipe();

    pid = fork();
    switch(pid){
        case -1:
            die_fork();
        case 0:
            if (fd_move(0, tochild[0]) == -1) die_fdmove();
            if (fd_move(1, fromchild[1]) == -1) die_fdmove();

            i = 0;
            run[i++] = "netclient";
            run[i++] = "-m";
            run[i++] = "-T30";
            run[i++] = "-C";
            if (keydir) {
                run[i++] = "-k";
                run[i++] = keydir;
            }
            run[i++] = host.s;
            run[i++] = port.s;
            run[i++] = "fdcopy";
            run[i++] = 0;

            sig_unblock(sig_pipe);
            pathexec_run(*run,run,envp);
            byte_zero(messagebuf, sizeof messagebuf);
            uint64_pack(messagebuf, errno);
            if (write(1, messagebuf, sizeof messagebuf) == -1) {};
            die_exec(run);
            break;
        default:
            close(tochild[0]);
            close(fromchild[1]);
            break;
    }

    r = timeoutread(40, fromchild[0],messagebuf,sizeof messagebuf);
    if (r != sizeof messagebuf) {
        if (r == -1) kill(pid,sig_term);
        wait_nohang(&wstatus);
        errno = error_pipe;
        die_child();
    }

    http_log_start();

    uint64_unpack(messagebuf, (uint64 *)&errnum);
    if (errnum != 0) {
        errno = errnum;
        wait_nohang(&wstatus);
        die_child();
    }
    if (messagebuf[8] == 'C') flagcurvecp = 1;
    ipstr = iptostr(0, (unsigned char *)messagebuf + 10);

    if (flagconnect) {
        goto connect;
    }

    buffer_init(&clientout, buffer_unixwrite, tochild[1], clientoutbuf, sizeof clientoutbuf);
    buffer_init(&serverin, serversaferead, fromchild[0], serverinbuf, sizeof serverinbuf);


    clientout_put(header.s, header.len);

    if (flagrequestbody){

        if (flagchunked){
            for(;;){
                clientreadline();
                clientout_put(line.s, line.len);
                clientout_puts("\r\n");
                if (!stralloc_0(&line)) die_nomem();
                i = scan_xlong(line.s,&ul);
                if (!i) {errno = error_proto; die_clientread();}
                if (ul == 0) break;
                clientcopyall(ul);
                clientreadline();
                clientout_put(line.s, line.len);
                clientout_puts("\r\n");
                if (line.len) {errno = error_proto; die_clientread();}
            }
            goto finish;
        }
        if (cl != -1){
            clientcopyall(cl);
            goto finish;
        }

        die_badrequest();
    }


finish:
    clientout_flush();
    response();
    http_log_access(ipstr, &host, &method, &logurl, &status, &statusmessage, &header, flagcurvecp, serverbytes);
    goto wait;

connect:
    if (protocolnum == 1)
        serverout_puts("HTTP/1.0 200 go ahead\r\n");
    else
        serverout_puts("HTTP/1.1 200 go ahead\r\n");
    if (flagcurvecp) {
        serverout_puts("X-CurveCP: true\r\n");
    }
    serverout_puts("\r\n");
    serverout_flush();
    if (!stralloc_copys(&status, "200")) die_nomem();
    if (!stralloc_copys(&statusmessage, "go ahead")) die_nomem();
    http_log_access(ipstr, &host, &method, &logurl, &status, &statusmessage, &header, flagcurvecp, 0);
    if (fdcp(fatal,fromchild[0],1,-1,0,tochild[1],-1) == -1) _exit(111);
    goto wait;

wait:
    /* XXX */
    for(i = 0; i < 10; ++i) {
        if (wait_nohang(&wstatus) == pid) return 0;
        deepsleep(1);
    }
    kill(pid,sig_term);
    return wait_pid(&wstatus,pid);
}
