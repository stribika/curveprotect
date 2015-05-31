#include "auto_home.h"

void hier()
{
  h(auto_home,-1,-1,02755);
  d(auto_home,"bin",-1,-1,02755);

  c(auto_home,"bin","tcpserver",-1,-1,0755);
  c(auto_home,"bin","tcprules",-1,-1,0755);
  c(auto_home,"bin","tcprulescheck",-1,-1,0755);
  c(auto_home,"bin","argv0",-1,-1,0755);
  c(auto_home,"bin","recordio",-1,-1,0755);
  c(auto_home,"bin","tcpclient",-1,-1,0755);
  c(auto_home,"bin","who@",-1,-1,0755);
  c(auto_home,"bin","date@",-1,-1,0755);
  c(auto_home,"bin","finger@",-1,-1,0755);
  c(auto_home,"bin","http@",-1,-1,0755);
  c(auto_home,"bin","tcpcat",-1,-1,0755);
  c(auto_home,"bin","mconnect",-1,-1,0755);
  c(auto_home,"bin","mconnect-io",-1,-1,0755);
  c(auto_home,"bin","addcr",-1,-1,0755);
  c(auto_home,"bin","delcr",-1,-1,0755);
  c(auto_home,"bin","fixcrio",-1,-1,0755);
  c(auto_home,"bin","rblsmtpd",-1,-1,0755);
}
