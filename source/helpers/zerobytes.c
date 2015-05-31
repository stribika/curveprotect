/*
20121218
Jan Mojzis
Public domain.
*/

#define FATAL "zerobytes: fatal: "

#define USAGE "\
\n\
zerobytes: usage:\n\
\n\
 name:\n\
   zerobytes - print zero byte-stream\n\
\n\
 syntax:\n\
   zerobytes [options] length\n\
\n\
 description:\n\
   zerobytes - prints zero byte-stream\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 arguments:\n\
   length (mandatory): byte-stream length\n\
\n\
 example:\n\
   zerobytes 1048576 > barrel.tmp && mv -f barrel.tmp barrel.dat\n\
\n\
"

void xfce(void *x, long long len) { return ; }

#include "xbytes.c"

