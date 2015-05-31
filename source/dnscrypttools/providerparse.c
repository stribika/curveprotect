#include "providerparse.h"
#include "dns.h"
#include "str.h"
#include "e.h"
#include "stralloc.h"

int providerparse(stralloc *name, unsigned char **q, const char *x) {

    if (!x) {errno = EPROTO; return 0; }
    if (!dns_domain_fromdot(q, (unsigned char *)x, str_len(x))) return 0;
    if (!stralloc_copys(name, "")) return 0;
    if (!dns_domain_todot_cat(name, *q)) return 0;
    return 1;
}
