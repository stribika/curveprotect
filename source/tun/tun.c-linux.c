#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>


#include <stdio.h>

int main(){

  struct ifreq ifr, netifr;

  ifr.ifr_flags  = IFF_TUN | IFF_NO_PI;
  netifr.ifr_mtu = 1024;

  printf("/* Public domain. */\n\n");
  printf("#include \"tun.h\"\n");
  printf("#include <net/if.h>\n");
  printf("#include <linux/if_tun.h>\n");
  printf("#include <stdlib.h>\n");
  printf("#include <sys/ioctl.h>\n");
  printf("#include <unistd.h>\n");
  printf("#include \"open.h\"\n");
  printf("#include \"stralloc.h\"\n");
  printf("#include \"byte.h\"\n");
  printf("#include \"forkexecwait.h\"\n");
  printf("#include \"ip4.h\"\n");
  printf("\n");
  printf("int tun_init(stralloc *name){\n");
  printf("\n");
  printf("    int fd;\n");
  printf("    struct ifreq ifr;\n");
  printf("\n");
  printf("    fd = open_readwrite(\"/dev/net/tun\");\n");
  printf("    if (fd == -1) return -1;\n");
  printf("\n");
  printf("    byte_zero(&ifr, sizeof ifr);\n");
  printf("    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;\n");
  printf("\n");
  printf("    if((ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0){\n");
  printf("        close(fd);\n");
  printf("        return -1;\n");
  printf("    }\n");
  printf("    if (!stralloc_copys(name, ifr.ifr_name)) {close(fd);return -1;}\n");
  printf("    if (!stralloc_0(name)) {close(fd);return -1;}\n");
  printf("    return fd;\n");
  printf("}\n\n");
  printf("int tun_ip4(stralloc *name, const char ip[4], const char gw[4]){\n");
  printf("\n");
  printf("    char ipstr[IP4_FMT];\n");
  printf("    char gwstr[IP4_FMT];\n");
  printf("    char *run[10];\n\n");
  printf("    ipstr[ip4_fmt(ipstr,ip)] = 0;\n");
  printf("    gwstr[ip4_fmt(gwstr,gw)] = 0;\n\n");
  printf("    run[0] = \"ifconfig\";\n");
  printf("    run[1] = name->s;\n");
  printf("    run[2] = \"promisc\";\n");
  printf("    run[3] = ipstr;\n");
  printf("    run[4] = \"pointopoint\";\n");
  printf("    run[5] = gwstr;\n");
  printf("    run[6] = \"mtu\";\n");
  printf("    run[7] = \"1024\";\n");
  printf("    run[8] = \"up\";\n");
  printf("    run[9] = 0;\n");
  printf("\n");
  printf("    return forkexecwait(run);\n");
  printf("}\n\n");
  printf("int tun_route4(stralloc *name, const char net[4], const char mask[4], const char gw[4]){\n");
  printf("\n");
  printf("    char netstr[IP4_FMT];\n");
  printf("    char gwstr[IP4_FMT];\n");
  printf("    char maskstr[IP4_FMT];\n");
  printf("    char *run[9];\n\n");
  printf("    maskstr[ip4_fmt(maskstr,mask)] = 0;\n");
  printf("    netstr[ip4_fmt(netstr,net)] = 0;\n");
  printf("    gwstr[ip4_fmt(gwstr,gw)] = 0;\n\n");
  printf("    run[0] = \"route\";\n");
  printf("    run[1] = \"add\";\n");
  printf("    run[2] = \"-net\";\n");
  printf("    run[3] = netstr;\n");
  printf("    run[4] = \"netmask\";\n");
  printf("    run[5] = maskstr;\n");
  printf("    run[6] = \"gw\";\n");
  printf("    run[7] = gwstr;\n");
  printf("    run[8] = 0;\n");
  printf("\n");
  printf("    return forkexecwait(run);\n");
  printf("}\n");

  return 0;
}

