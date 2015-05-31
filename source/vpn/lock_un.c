/* Public domain. */

#include <unistd.h>
#include "lock.h"

int lock_un(int fd) { return lockf(fd,0,0); }
