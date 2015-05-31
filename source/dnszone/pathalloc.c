#include "alloc.h"
#include "byte.h"
#include "str.h"
#include "pathalloc.h"

char *pathalloc(const char *name1, const char *name2, const char *name3) {

    char *x, *p;
    long long namelen1 = 0, namelen2 = 0, namelen3 = 0, len = 1;

    if (name1) {namelen1 = str_len(name1); len += namelen1 + 1;}
    if (name2) {namelen2 = str_len(name2); len += namelen2 + 1;}
    if (name3) {namelen3 = str_len(name3); len += namelen3 + 1;}

    x = (char *)alloc(len);
    if (!x) return x;
    p = x; *p = 0;

    if (!name1) return x;
    byte_copy(p, namelen1, name1); p += namelen1; *p = 0;

    if (!name2) return x;
    *p++ = '/';
    byte_copy(p, namelen2, name2); p += namelen2; *p = 0;

    if (!name3) return x;
    *p++ = '/';
    byte_copy(p, namelen3, name3); p += namelen3; *p = 0;
   
    return x;
}

