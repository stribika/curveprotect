/*
20121218
Jan Mojzis
Public domain.
*/

#include "randombytes.h"

#define xfce randombytes
#define FATAL "randombytes: fatal: "

#define USAGE "\
\n\
randombytes: usage:\n\
\n\
 name:\n\
   randombytes - print random byte-stream\n\
\n\
 syntax:\n\
   randombytes [options] length\n\
\n\
 description:\n\
   randombytes - prints random byte-stream\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 arguments:\n\
   length (mandatory): byte-stream length\n\
\n\
 example:\n\
   randombytes 1048576 > barrel.tmp && mv -f barrel.tmp barrel.dat\n\
\n\
"

#include "xbytes.c"

