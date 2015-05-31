#include <unistd.h>
#include "e.h"
#include "byte.h"
#include "uint64_pack.h"
#include "uint64_unpack.h"
#include "writeall.h"
#include "statusmessage.h"


static int readall(int fd,void *xv,long long xlen)
{
  long long r;
  char *x=xv;
  while (xlen > 0) {
    r = xlen;
    if (r > 1048576) r = 1048576;
    r = read(fd,x,r);
    if (r == 0) errno = EPROTO;
    if (r <= 0) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
      return -1;
    }
    x += r;
    xlen -= r;
  }
  return 0;
}

int statusmessage_write(int fd, long long e, char ch, unsigned char *ip, unsigned char *port) {

    unsigned char message[32];

    byte_zero(message, sizeof message);
    uint64_pack(message, e);
    message[8] = ch; /* T ... TCP, C ... CurveCP*/
    message[9] = 'X'; /* TODO: IPv4 or IPv6 ??? */
    byte_copy(message + 10, 16, ip);
    byte_copy(message + 26, 2, port);
    return writeall(fd, message, sizeof message);
}

int statusmessage_read(int fd, long long *e, int *flagcurvecp, unsigned char *ip, unsigned char *port) {

    unsigned char message[32];

    if (readall(fd, message, sizeof message) == -1) return -1;

    *e = uint64_unpack(message);
    *flagcurvecp = 0;
    if (message[8] == 'C') *flagcurvecp = 1;

    byte_copy(ip, 16, message + 10);
    byte_copy(port, 2, message + 26);
    if (!byte_isequal(message + 28, 4, "\0\0\0\0")) { errno = EPROTO; return -1; }
    return 0;
}
