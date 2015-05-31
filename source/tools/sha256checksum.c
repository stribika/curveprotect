/*
20130113
Jan Mojzis
Public domain.
*/


#include "sha256.h"

#define sha sha256
#define sha_init sha256_init
#define sha_block sha256_block
#define sha_last sha256_last
#define sha_ctx sha256_ctx
#define BYTES sha256_BYTES

#define FATAL "sha256checksum: fatal: "

#define USAGE "\
\n\
sha256checksum: usage:\n\
\n\
 name:\n\
   sha256checksum - compute SHA256 checksum\n\
\n\
 syntax:\n\
   sha256checksum [options] [number] <input\n\
\n\
 description:\n\
   sha256checksum reads input byte-stream and prints hexadecimal SHA256 checksum\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 arguments:\n\
   number (optional): truncate output to 'number' bytes\n\
\n\
 example:\n\
   sha256checksum </dev/null\n\
   sha256checksum 16 </dev/null\n\
\n\
"

#include "checksum.c"
