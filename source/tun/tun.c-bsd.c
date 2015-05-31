#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if_tun.h>
#include <net/if.h>

void test(void) { 
    int fd = -1;
    int i = IFF_POINTOPOINT|IFF_MULTICAST;
    ioctl(fd, TUNSIFMODE, &i);
    ioctl(fd, TUNSLMODE, &i);
    ioctl(fd, TUNSIFHEAD, &i);
    return;
}

int main(int argc, char **argv) {

  printf("/* Public domain. */\n\n");
  printf("#include \"tun.h\"\n");
  printf("#include <sys/types.h>\n");
  printf("#include <sys/socket.h>\n");
  printf("#include <sys/ioctl.h>\n");
  printf("#include <net/if_tun.h>\n");
  printf("#include <net/if.h>\n");
  printf("#include <unistd.h>\n");
  printf("#include \"open.h\"\n");
  printf("#include \"stralloc.h\"\n");
  printf("#include \"byte.h\"\n");
  printf("#include \"forkexecwait.h\"\n");
  printf("#include \"ip4.h\"\n\n");
  printf("\n");
  printf("stralloc dev = {0};\n");
  printf("\n");
  printf("int tun_init(stralloc *name){\n");
  printf("\n");
  printf("    int fd;\n");
  printf("    int i;\n");
  printf("\n");
  printf("    for(i = 0; i < 256; ++i){\n");
  printf("        if (!stralloc_copys(name,\"tun\" )) return -1;\n");
  printf("        if (!stralloc_catint(name, i)) return -1;\n");
  printf("        if (!stralloc_0(name)) return -1;\n");
  printf("        if (!stralloc_copys(&dev,\"/dev/\")) return -1;\n");
  printf("        if (!stralloc_cat(&dev, name)) return -1;\n");
  printf("        fd = open_readwrite(dev.s);\n");
  printf("        if (fd != -1) {\n");
  printf("            i = IFF_POINTOPOINT|IFF_MULTICAST;\n");
  printf("            if (ioctl(fd, TUNSIFMODE, &i) < 0) {close(fd); return -1;}\n");
  printf("            i = 0;\n");
  printf("            if (ioctl(fd, TUNSLMODE, &i) < 0) {close(fd); return -1;}\n");
  printf("            i = 0;\n");
  printf("            if (ioctl(fd, TUNSIFHEAD, &i) < 0) {close(fd); return -1;}\n");
  printf("            return fd;\n");
  printf("        }\n");
  printf("    }\n");
  printf("    return -1;\n");
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
  printf("    run[2] = ipstr;\n");
  printf("    run[3] = gwstr;\n");
  printf("    run[4] = \"mtu\";\n");
  printf("    run[5] = \"1024\";\n");
  printf("    run[6] = \"netmask\";\n");
  printf("    run[7] = \"255.255.255.255\";\n");
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
  printf("    run[4] = \"-interface\";\n");
  printf("    run[5] = name->s;\n");
  printf("    run[6] = maskstr;\n");
  printf("    run[7] = 0;\n");
  printf("\n");
  printf("    return forkexecwait(run);\n");
  printf("}\n");

  return 0;
}

