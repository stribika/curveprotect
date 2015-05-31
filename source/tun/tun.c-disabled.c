#include <stdio.h>

int main(){
  printf("/* Public domain. */\n\n");
  printf("#include \"stralloc.h\"\n\n");
  printf("#include \"error.h\"\n\n");
  printf("int tun_ip4(stralloc *name, const char ip[4], const char gw[4]){return -1;}\n");
  printf("int tun_route4(stralloc *name, const char net[4], const char mask[4], const char gw[4]){return -1;}\n");
  printf("int tun_init(stralloc *name){\n");
  printf("    errno = error_nodevice;\n");
  printf("    return -1;\n");
  printf("}\n");

  return 0;
}
