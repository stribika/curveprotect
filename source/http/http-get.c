/*
 * 20100504
 * Jan Mojzis
 * Public domain.
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include "strerr.h"
#include "buffer.h"
#include "timeoutread.h"
#include "timeoutwrite.h"
#include "fmt.h"
#include "open.h"
#include "getln.h"
#include "stralloc.h"
#include "byte.h"
#include "scan.h"
#include "case.h"
#include "error.h"
#include "tai.h"
#include "httpdateparse.h"
#include "httpdate.h"
#include "utai.h"
#include "env.h"

#define FATAL "http-get: fatal: "

const char *path;
const char *fn;
const char *tmpfn;

void unlinktmp(void){

    int savederrno = errno;
    unlink(tmpfn);
    errno = savederrno;
    return;
}

void usage(void){strerr_die1x(100,"http-get: usage: http-get http://host/path fn tmpfn");}

void die_open(const char *str){unlinktmp(); strerr_die4sys(111,FATAL,"unable to open file '",str,"': ");}
void die_stat(const char *str){unlinktmp(); strerr_die4sys(111,FATAL,"unable to stat file '",str,"': ");}
void die_netwrite(void){unlinktmp(); strerr_die2sys(111,FATAL,"unable to write to network: ");}
void die_netread(void){unlinktmp(); strerr_die2sys(111,FATAL,"unable to read from network: ");}
void die_nomem(void){unlinktmp(); strerr_die2x(111,FATAL,"out of memory");}
void die_write(const char *str){unlinktmp(); strerr_die4sys(111,FATAL,"unable to write file '",str,"': ");}
void die_rename(const char *str1, const char *str2){unlinktmp(); strerr_die6sys(111,FATAL,"unable to move ",str1," to ",str2,": ");}


buffer b;
char bspace[1024];
buffer out;
char outbuf[1024];
buffer in;
char inbuf[512];

int flagchunked = 0;
long long cl = -1;


int safewrite(int fd, char *buf, int len){

    int r;
    r = timeoutwrite(60, fd, buf, len);
    if (r <= 0) die_netwrite();
    return r;
}

buffer out = BUFFER_INIT(safewrite, 7, outbuf, sizeof outbuf);

char strnum[FMT_ULONG];
void out_put(char *s, int len){buffer_put(&out, s, len);}
void out_puts(const char *s){buffer_puts(&out, s);}
void out_putnum(unsigned long num){out_put(strnum,fmt_ulong(strnum,num));}
void out_flush(void){buffer_flush(&out);}

int saferead(int fd, char *buf, int len){

    int r;
    out_flush();
    r = timeoutread(60,fd,buf,len);
    if (r == -1) die_netread();
    return r;
}

buffer in = BUFFER_INIT(saferead, 6, inbuf, sizeof inbuf);


stralloc line = {0};
stralloc version = {0};
stralloc status = {0};
stralloc statusmessage = {0};
stralloc xpath = {0};
stralloc url  = {0};
stralloc host  = {0};
stralloc field = {0};
stralloc sacl = {0};
stralloc salm = {0};
struct tai tailm;
struct tai now;
int flaglm = 0;
stralloc mtimestr = {0};
struct tai mtime;

void readline(void){

    int match;

    if (getln(&in,&line,&match,'\n') == -1) die_netread();
    if (!match) die_netread();
    if (line.len && (line.s[line.len - 1] == '\n')) --line.len;
    if (line.len && (line.s[line.len - 1] == '\r')) --line.len;
    line.s[line.len] = 0;
    return;
}

char buf[1024];

void copyall(unsigned long long size){

    int r,w;

    while(size > 0){
        if (size > sizeof buf)
            r = buffer_get(&in, buf, sizeof buf);
        else
            r = buffer_get(&in, buf, size);
        if (r == 0){
            errno = error_proto; /* server closed connection */
            die_netread();
        }
        if (r == -1)die_netread();
        size -= r;

        w = buffer_put(&b, buf, r);
        if (w == -1) die_write(tmpfn);
    }

}

const char *proxy = (const char *)0;


int main(int argc,char **argv){

    struct stat st;
    int fd;
    unsigned int i, spaces;
    unsigned long u = 0;
    unsigned long long ull = 0;

    proxy = env_get("PROXYHOST");

    path = argv[1];
    if (!path) usage();
    if (path[0] != '/' && path[0] != 'h') usage();
    fn = argv[2];
    if (!fn) usage();
    tmpfn = argv[3];
    if (!tmpfn) usage();


    if (stat(fn, &st) == 0){ 
	tai_unix(&mtime,st.st_mtime);
	if (!httpdate(&mtimestr,&mtime)) die_nomem();
    }


    if (!stralloc_copys(&xpath,path)) die_nomem();
    if (case_startb(xpath.s,xpath.len,"http://")) {
      if (!stralloc_copyb(&host,xpath.s + 7,xpath.len - 7)) die_nomem();
      i = byte_chr(host.s,host.len,'/');
      if (!stralloc_copyb(&url,xpath.s + 7 + i, xpath.len - 7 - i)) die_nomem();
      host.len = i;
      if (!stralloc_0(&host)) die_nomem();
    }
    else{
	if (!stralloc_copy(&url, &xpath)) die_nomem();
    }
    if (!stralloc_0(&url)) die_nomem();

    if (!host.len) usage();

    out_puts("GET ");

    if (proxy){
        out_puts("http://");
        out_puts(host.s);
        out_puts(url.s);
    }
    else{
        out_puts(url.s);
    }

    out_puts(" HTTP/1.1\r\nHost: ");
    out_puts(host.s);
    out_puts("\r\nConnection: close\r\n");
    if (proxy){
      out_puts("Proxy-Connection: close\r\n");
    }
    if (mtimestr.len){
	out_puts("If-Modified-Since:");
	out_put(mtimestr.s, mtimestr.len);
	out_puts("\r\n");
    }
    out_puts("\r\n");
    out_flush();

    readline();

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

    if (status.len != 3) {
        if (!stralloc_0(&status)) die_nomem();
        if (!stralloc_0(&statusmessage)) die_nomem();
        strerr_die5x(111,FATAL,"http GET request failed: ",status.s," ", statusmessage.s);
    }

    if (byte_equal(status.s, status.len, "304")){
	_exit(0);
    }

    if (byte_diff(status.s, status.len, "200")){
	if (!stralloc_0(&status)) die_nomem();
	if (!stralloc_0(&statusmessage)) die_nomem();
	strerr_die5x(111,FATAL,"http GET request failed: ",status.s," ", statusmessage.s);
    }


    if (!stralloc_copys(&field,"")) die_nomem();
    for (;;) {
        readline();
        if (!line.len || ((line.s[0] != ' ') && (line.s[0] != '\t'))){

                /* header Content-Length: */
                if (case_startb(field.s,field.len,"content-length: ")){
                    if (!stralloc_copyb(&sacl,field.s + 16,field.len - 16)) die_nomem();
                    if (!stralloc_0(&sacl)) die_nomem();
                    if (scan_ulonglong(sacl.s,&ull)){
			 cl = ull;
		    }
                }
                /* header Last-Modified: */
                if (case_startb(field.s,field.len,"last-modified: ")){
                    if (!stralloc_copyb(&salm,field.s + 15,field.len - 15)) die_nomem();
		    if (!httpdateparse(&salm,&tailm)){
			flaglm = 1;
		    }
                }
                /* header Transfer-Encoding: */
                if (case_startb(field.s,field.len,"transfer-encoding: chunked")){
		    flagchunked = 1;
                }

                field.len = 0;
        }
        if (!line.len) break;
        if (!stralloc_cat(&field,&line)) die_nomem();
    }

    fd = open_trunc(tmpfn);
    if (fd == -1) die_open(tmpfn);
    buffer_init(&b,buffer_unixwrite,fd,bspace,sizeof bspace);

    if (flagchunked){
	for(;;){
	    readline();
	    if (!stralloc_0(&line)) die_nomem();
	    i = scan_xlong(line.s,&u);
	    if (!i) {errno = error_proto; die_netread();}
	    if (u == 0) break;
	    copyall(u);
	    readline();
	    if (line.len) {errno = error_proto; die_netread();}
	}
	goto finish;
    }

    if (cl != -1){
	copyall(cl);
	goto finish;

    }

    /* XXX - expecting that server closes connection */
    switch(buffer_copy(&b, &in)) {
	case -2:
	    die_netread();
	case -3:
	    die_write(tmpfn);
    }

finish:

    if (buffer_flush(&b) == -1) die_write(tmpfn);
    if (fsync(fd) == -1) die_write(tmpfn);
    if (close(fd) == -1) die_write(tmpfn);

    if (flaglm){
	tai_now(&now);
	if (utai(tmpfn,&tailm,&now) == -1) die_write(tmpfn);
    }

    if (rename(tmpfn, fn) == -1) die_rename(tmpfn, fn);
    _exit(0);
}
