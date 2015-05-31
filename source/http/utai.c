#include "utai.h"
#include <sys/types.h>
#include <utime.h>

int utai(const char *fn, struct tai *mtime, struct tai *atime){

    struct utimbuf ubuf;
    struct tai x;

    tai_unix(&x,0);
    tai_sub(&x,atime,&x);
    ubuf.actime  = tai_approx(&x);

    tai_unix(&x,0);
    tai_sub(&x,mtime,&x);
    ubuf.modtime = tai_approx(&x);

    return utime(fn,&ubuf);
}
