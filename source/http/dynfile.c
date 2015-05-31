#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "case.h"
#include "stralloc.h"
#include "error.h"
#include "str.h"
#include "wait.h"
#include "open.h"
#include "fd.h"
#include "pathexec.h"
#include "env.h"
#include "buffer.h"
#include "fmt.h"
#include "sig.h"


static char strnum[FMT_ULONG];

static void httplog(char *fn, char *qs, int flagdyn, const char *r1, const char *r2){
    int i;
    char ch;
    char *x;

    x = env_get("TCPREMOTEIP");
    if (!x) x = "0";
    buffer_puts(buffer_2,x);

    if (flagdyn){
      buffer_puts(buffer_2, " d: ");
    }
    else{
      buffer_puts(buffer_2, " s: ");
    }

    for (i = 0;i < 100;++i) {
        ch = fn[i];
        if (!ch) break;
        if (ch < 32) ch = '?';
        if (ch > 126) ch = '?';
        if (ch == ' ') ch = '_';
        buffer_put(buffer_2,&ch,1);
    }
    if (i == 100)
        buffer_puts(buffer_2,"...");

    if (str_len(qs)){

        buffer_puts(buffer_2,"?");

        for (i = 0;i < 100;++i) {
            ch = qs[i];
            if (!ch) break;
            if (ch < 32) ch = '?';
            if (ch > 126) ch = '?';
            if (ch == ' ') ch = '_';
            buffer_put(buffer_2,&ch,1);
        }
        if (i == 100)
            buffer_puts(buffer_2,"...");
    }

    buffer_puts(buffer_2," ");
    buffer_puts(buffer_2,r1);
    buffer_puts(buffer_2,r2);
    buffer_puts(buffer_2,"\n");
    buffer_flush(buffer_2);
}




int dynfile(char *fn, char *qs, char *tmp, stralloc *contenttype, unsigned long *length){

    const char *result;
    char *x;
    struct stat st;
    int flagdyn = 0;
    int pid, status;
    int fd,r;
    char *run[3];

    x = fn + str_rchr(fn,'.');
    result = "text/plain";
    flagdyn = 0;
         if (!case_diffs(x,".html"))     result = "text/html";
    else if (!case_diffs(x,".dynhtml"))  {result = "text/html"; flagdyn = 1;}
    else if (!case_diffs(x,".css"))      result = "text/css";
    else if (!case_diffs(x,".dyncss"))   {result = "text/css"; flagdyn = 1;}
    else if (!case_diffs(x,".js"))       result = "application/javascript";
    else if (!case_diffs(x,".dynjs"))    {result = "application/javascript"; flagdyn = 1;}
    else if (!case_diffs(x,".gif"))      result = "image/gif";
    else if (!case_diffs(x,".jpeg"))     result = "image/jpeg";
    else if (!case_diffs(x,".jpg"))      result = "image/jpeg";
    else if (!case_diffs(x,".png"))      result = "image/png";
    else if (!case_diffs(x,".enc20"))    {result = "application/octet-stream"; flagdyn = 1;} /* XXX - hack for backups */

    if (!stralloc_copys(contenttype,"Content-Type: ")) return -1;
    if (!stralloc_cats(contenttype,result)) return -1;
    if (!stralloc_cats(contenttype,"\r\n")) return -1;
    
    if (stat(fn,&st) == -1){
        httplog(fn, qs, flagdyn, "404 ",error_str(errno));
        return -1;
    }

    if ((st.st_mode & 0444) != 0444){
        errno = error_acces;
        httplog(fn, qs, flagdyn, "404 ",error_str(errno));
        return -1;
    }
    if ((st.st_mode & 0101) == 0001){
        errno = error_acces;
        errno = error_acces;
        httplog(fn, qs, flagdyn, "404 ",error_str(errno));
        return -1;
    }
    if ((st.st_mode & S_IFMT) != S_IFREG){
        errno = error_acces;
        errno = error_acces;
        httplog(fn, qs, flagdyn, "404 ",error_str(errno));
        return -1;
    }

    if (!flagdyn){
        *length = st.st_size;
        httplog(fn, qs, flagdyn, "200 ","OK");
        return open_read(fn);
    }
    
    fd = open_trunc(tmp);
    if (fd == -1){
        httplog(fn, qs, flagdyn, "404 ",error_str(errno));
        return -1;
    }

    pid = fork();

    switch(pid){
        case -1:
            httplog(fn, qs, flagdyn, "500 ",error_str(errno));
            close(fd);
            return -2;
            break;
        case 0:
            if (!pathexec_env("QUERY_STRING", qs)) _exit(21);
            if (fd_move(1,fd) == -1) _exit(111); 
            run[0] = fn;
            run[1] = qs;
            run[2] = 0;
            pathexec(run);
            _exit(111);
            break;
        default:
            break;
    }

    r = wait_pid(&status,pid);
    if (r == -1){
        httplog(fn, qs, flagdyn, "500 ",error_str(errno));
        close(fd);
        unlink(tmp);
        return -2;
    }

    r = wait_crashed(status);
    if (r) {
        httplog(fn, qs, flagdyn, "500 ", sig_str(r));
        close(fd);
        unlink(tmp);
        return -2;
    }

    r = wait_exitcode(status);
    if (r) {
        strnum[fmt_ulong(strnum,r)] = 0;
        httplog(fn, qs, flagdyn, "500 child exited with exitcode ", strnum);
        close(fd);
        unlink(tmp);
        return -2;
    }

    if (fstat(fd,&st) == -1){
        httplog(fn, qs, flagdyn, "500 ",error_str(errno));
        close(fd);
        unlink(tmp);
        return -2;
    }
    close(fd);

    *length = st.st_size;
    httplog(fn, qs, flagdyn, "200 ","OK");
    return open_read(tmp);

}
