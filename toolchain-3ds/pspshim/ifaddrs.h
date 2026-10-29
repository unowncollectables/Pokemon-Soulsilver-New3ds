/* ss3ds: libntrwifi's CPS (cps/src/network.c, SDK_BUILD_LINUX branch) finds the local IP with getifaddrs(). The 3DS has no
   such call; soulsilver3ds/platform/ss3ds_net.c provides getifaddrs/freeifaddrs on top of gethostid(). Keep the struct
   in sync with the copy in ss3ds_net.c. */
#ifndef SS3DS_IFADDRS_H
#define SS3DS_IFADDRS_H
#include <sys/socket.h>
struct ifaddrs {
    struct ifaddrs *ifa_next;
    char *ifa_name;
    unsigned int ifa_flags;
    struct sockaddr *ifa_addr;
    struct sockaddr *ifa_netmask;
};
int getifaddrs(struct ifaddrs **ifap);
void freeifaddrs(struct ifaddrs *ifa);
#endif
