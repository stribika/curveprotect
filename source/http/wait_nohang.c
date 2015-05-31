#include <sys/types.h>
#include <sys/wait.h>

int wait_nohang(int *wstat)
{
  return waitpid(-1,wstat,WNOHANG);
}

