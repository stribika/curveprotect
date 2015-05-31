/*
20130113
Jan Mojzis
Public domain.
*/


#include "sha512.h"

#define sha sha512
#define sha_init sha512_init
#define sha_block sha512_block
#define sha_last sha512_last
#define sha_ctx sha512_ctx
#define BYTES sha512_BYTES

#define FATAL "sha512checksum: fatal: "

#define USAGE "\
\n\
sha512checksum: usage:\n\
\n\
 name:\n\
   sha512checksum - compute SHA512 checksum\n\
\n\
 syntax:\n\
   sha512checksum [options] [number] <input\n\
\n\
 description:\n\
   sha512checksum reads input byte-stream and prints hexadecimal SHA512 checksum\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 arguments:\n\
   number (optional): truncate output to 'number' bytes\n\
\n\
 example:\n\
   sha512checksum </dev/null\n\
   sha512checksum 16 </dev/null\n\
\n\
"

#include "checksum.c"
