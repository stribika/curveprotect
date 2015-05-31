/* Public domain. */

#include <unistd.h>
#include "lock.h"

int lock_ex(int fd) { return lockf(fd,1,0); }
