#include "pathexec.h"
#include "strerr.h"

main(int argc,char **argv,char **envp)
{
  if (argc < 3)
    strerr_die1x(100,"argv0: usage: argv0 realname program [ arg ... ]");
  pathexec_run(argv[1],argv + 2,envp);
  strerr_die4sys(111,"argv0: fatal: ","unable to run ",argv[1],": ");
}
