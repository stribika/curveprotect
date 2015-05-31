/*
 * 20090902
 * Jan Mojzis
 * Public domain.
 */

#ifndef _HTTP_H____
#define _HTTP_H____

#include "stralloc.h"

extern const char *http_str(int i);
extern void http_log_access(const char *ip, stralloc *host, stralloc *method, stralloc *logurl, stralloc *status, stralloc *statusmessage, stralloc *header, int curvecpflag, unsigned long long bytes);
extern void http_log_debug(stralloc *header);
extern void http_log_fatal(stralloc *header, const char **s);
extern void http_log_start(void);

#endif

