#include "buffer.h"
#include "http.h"
#include "scan.h"
#include "fmt.h"
#include "taia.h"
#include <unistd.h>

struct taia start;

void http_log_start(void) {
    taia_now(&start);
    return;
}


void safeput(char *s, unsigned int size){

#define min(x, y) ((x) < (y) ? (x) : (y))
#define MAXRECORDSIZE 200

    int i;
    char ch;
    unsigned int len;

    if (!size){
        buffer_puts(buffer_2, "- ");
        return;
    }

    len = min(MAXRECORDSIZE,size);

    for (i = 0;i < len;++i) {
        ch = *(s + i);
        if (ch == 0 || ch == ' ' || ch == '\n' || ch == '\''){
            ch = '_';
        }
        if (ch < 32 || ch > 126){
            ch = '?';
        }
        buffer_PUTC(buffer_2, ch);
    }
    if (len != size){
        buffer_puts(buffer_2, "...");
    }
    return;
}

static char strnum[FMT_ULONG];
unsigned int strnum_len;

void http_log_access(const char *ip, stralloc *host, stralloc *method, stralloc *logurl, stralloc *status, stralloc *statusmessage, stralloc *header, int curvecpflag, unsigned long long bytes){

    unsigned long u,ux;
    unsigned long long uu;
    struct taia end;

    taia_now(&end);
    taia_sub(&end,&end, &start);
    uu = 1000 * end.sec.x + end.nano / 1000000;

    buffer_puts(buffer_2, "[");
    buffer_put(buffer_2, strnum,fmt_uint0(strnum,getpid(),5));
    if(curvecpflag){
        buffer_puts(buffer_2, "] access: C: ");
    }
    else{
        buffer_puts(buffer_2, "] access: H: ");
    }
    /* ip */
    safeput(host->s, host->len);
    buffer_puts(buffer_2, "(");
    buffer_puts(buffer_2, ip);
    buffer_puts(buffer_2, ") ");
    safeput(method->s, method->len);
    buffer_puts(buffer_2, " ");
    safeput(logurl->s, logurl->len);
    buffer_puts(buffer_2, " ");
    safeput(status->s, status->len);
    buffer_puts(buffer_2, " ");
    safeput(statusmessage->s, statusmessage->len);
    buffer_puts(buffer_2, " ");
    buffer_put(buffer_2, strnum,fmt_ulonglong(strnum,bytes));
    buffer_puts(buffer_2, " ");
    buffer_put(buffer_2, strnum,fmt_ulonglong(strnum,uu));
    buffer_puts(buffer_2, "\n");

    status->s[status->len] = 0;
    scan_ulong(status->s,&u);

    ux=u/100;

    if (ux != 2 && ux != 3 && u != 404 && u != 403){
        http_log_debug(header);
    }
    buffer_flush(buffer_2);
}

void http_log_debug(stralloc *header){

    unsigned int i;
    char ch;


    strnum_len = fmt_uint0(strnum,getpid(),5);

    buffer_puts(buffer_2, "[");
    buffer_put(buffer_2, strnum, strnum_len);
    buffer_puts(buffer_2, "] debug: ");
    for(i = 0; i < header->len; ++i){
        ch = header->s[i];
        if (ch == '\n'){
            buffer_puts(buffer_2, "\n[");
            buffer_put(buffer_2, strnum, strnum_len);
            buffer_puts(buffer_2, "] debug: ");
            continue;
        }
        if (ch == '\r') continue;
        if (ch == 0 || ch == ' ' || ch == '\''){
            ch = '_';
        }
        if (ch < 32 || ch > 126){
            ch = '?';
        }
        buffer_PUTC(buffer_2, ch);
    }
    buffer_puts(buffer_2, "\n");
    buffer_flush(buffer_2);
    return;
}

void http_log_fatal(stralloc *header, const char **s){


    unsigned int i;

    buffer_puts(buffer_2, "[");
    buffer_put(buffer_2, strnum,fmt_uint0(strnum,getpid(),5));
    buffer_puts(buffer_2, "] fatal: ");

    for(i = 0; s[i]; ++i){
        buffer_puts(buffer_2, s[i]);
    }
    buffer_puts(buffer_2, "\n");
    buffer_flush(buffer_2);
    http_log_debug(header);
    return;

}


