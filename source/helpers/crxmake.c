/*
20120729
Jan Mojzis
Public domain.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h> /* rename */
#include "die.h"
#include "e.h"
#include "writeall.h"
#include "uint32_pack.h"
#include "byte.h"
#include "open.h"

#define FATAL "crxmake: fatal: "

void die_usage(void) {
    die_1(100, "crxmake: usage: crxmake crxfile tmpfile publickeyfile signumfile zipfile\n");
}

unsigned char buf[1024];

int copy(long long outfd, long long infd) {

  long long r;

  for (;;) {
    r = read(infd, (char *)buf, sizeof buf);
    if (r == 0) break;
    if (r == -1) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
        return -2;
    }
    if (writeall(outfd, (char *)buf, r) == -1) return -3;
  }
  return 0;
}

long long fd[3];
long long tmpfd;

int main(int argc, char **argv) {

    long long i;
    struct stat st;

    if (!argv[0]) die_usage();
    if (!argv[1]) die_usage();
    if (!argv[2]) die_usage();
    if (!argv[3]) die_usage();
    if (!argv[4]) die_usage();
    if (!argv[5]) die_usage();

    /* magic */
    byte_copy(buf + 0, 4, "Cr24");

    /* version */
    uint32_pack(buf + 4, 2);

    /* public key length */
    if (stat(argv[3], &st) == -1) die_6(111, FATAL, "unable to stat file ", argv[3], ": ", e_str(errno), "\n");
    uint32_pack(buf + 8, st.st_size);

    /* signature length */
    if (stat(argv[4], &st) == -1) die_6(111, FATAL, "unable to stat file ", argv[4], ": ", e_str(errno), "\n");
    uint32_pack(buf + 12, st.st_size);

    /* open tmp file */
    tmpfd = open_trunc(argv[2]);
    if (tmpfd == -1) die_6(111, FATAL, "unable to open file ", argv[2], ": ", e_str(errno), "\n");

    /* write header */
    if (writeall(tmpfd, buf, 16) == -1) die_6(111, FATAL, "unable to write to file ", argv[2], ": ", e_str(errno), "\n");

    for(i = 0; i < 3; ++i) {
      fd[i] = open_read(argv[i + 3]);
      if (fd[i] == -1) die_6(111, FATAL, "unable to open file ", argv[i + 3], ": ", e_str(errno), "\n");
      switch(copy(tmpfd, fd[i])) {
          case -2: die_6(111, FATAL, "unable to read from file ", argv[i + 3], ": ", e_str(errno), "\n"); break;
          case -3: die_6(111, FATAL, "unable to write to file ", argv[2], ": ", e_str(errno), "\n"); break;
      }
    }

    if (fsync(tmpfd) == -1) die_6(111, FATAL, "unable to write to file ", argv[2], ": ", e_str(errno), "\n");
    if (close(tmpfd) == -1) die_6(111, FATAL, "unable to write to file ", argv[2], ": ", e_str(errno), "\n");
    if (rename(argv[2], argv[1]) == -1) die_8(111, FATAL, "unable to rename ", argv[2], " to ", argv[1], ": ", e_str(errno), "\n");
    _exit(0);
}
