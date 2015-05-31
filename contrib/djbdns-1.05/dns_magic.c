#include "dns.h"
#include "str.h"

unsigned char *dns_magicq = (unsigned char *)"Q6fnvWj8";
unsigned char *dns_magicr = (unsigned char *)"R6fnvWJ8";

void dns_magic_init(const char *m1, const char *m2) {

    if (m1 && str_len(m1) == 8) dns_magicq = (unsigned char *)m1;
    if (m2 && str_len(m2) == 8) dns_magicr = (unsigned char *)m2;
}
