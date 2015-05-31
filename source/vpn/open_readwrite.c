/* Public domain. */

#include <sys/types.h>
#include <fcntl.h>
#include "open.h"

int open_readwrite(const char *fn)
{ return open(fn,O_RDWR | O_NDELAY); }
