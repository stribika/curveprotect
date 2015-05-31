#include <poll.h>
#include <unistd.h>
#include "error.h"
#include "writeall.h"

int writeall(int fd,const void *xv,long long xlen)
{
  long long w;
  const char *x = xv;
  while (xlen > 0) {
    w = xlen;
    if (w > 1048576) w = 1048576;
    w = write(fd,x,w);
    if (w < 0) {
      if (errno == error_intr || errno == error_again || errno == error_wouldblock) {
        struct pollfd p;
	p.fd = fd;
	p.events = POLLOUT | POLLERR;
	poll(&p,1,-1);
        continue;
      }
      return -1;
    }
    x += w;
    xlen -= w;
  }
  return 0;
}
