#include <fcntl.h>
#include "coe.h"

void coe_enable(int fd) {
    fcntl(fd, F_SETFD, 1);
}

void coe_disable(int fd) {
    fcntl(fd, F_SETFD, 0);
}
