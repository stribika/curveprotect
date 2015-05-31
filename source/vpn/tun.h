#ifndef _TUN_H____
#define _TUN_H____

#include "stralloc.h"

extern int tun_init(stralloc *name);
extern int tun_ip4(stralloc *name, const char *ip, const char *gateway);
extern int tun_route4(stralloc *name, const char *net, const char *mask, const char *gateway);

#endif
