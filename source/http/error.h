/* Public domain. */

#ifndef _ERROR_H____
#define _ERROR_H____

#include <errno.h>

extern int error_intr;
extern int error_nomem;
extern int error_noent;
extern int error_txtbsy;
extern int error_io;
extern int error_exist;
extern int error_timeout;
extern int error_inprogress;
extern int error_wouldblock;
extern int error_again;
extern int error_pipe;
extern int error_perm;
extern int error_acces;
extern int error_nodevice;
extern int error_proto;
extern int error_isdir;
extern int error_connrefused;

extern int error_notdir;

extern const char *error_str(int);
extern int error_temp(int);

/* XXX */
#ifndef DNSNOENT
#define DNSNOENT (-7000)
#endif


#endif /* _ERROR_H____ */
