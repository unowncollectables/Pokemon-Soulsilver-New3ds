/* online trading M4 (docs/WIRELESS.md Option B; GTS first): 3DS sockets for the game's own Wi-Fi Connection code.
 * pspoke already links cybervisi0n's libntrwifi (CPS TCP/IP) and libntrdwc (login, GTS); patch 0137 builds them with the
 * Linux socket backend (real BSD sockets plus a select thread). That backend needs three things from the platform:
 *   1. sockets: socInit with a 0x100000 byte aligned buffer (P3DS_NetInit, called from wm_uds.c when wireless=2),
 *   2. the local IP: getifaddrs() below (the 3DS's own Wi-Fi connection, so WPA2 works; the IP is read by connecting a UDP
 *      socket towards the DNS server and asking getsockname, which needs no extra libctru calls),
 *   3. the DNS server: the community Wi-Fi Connection DNS answers nas/gamestats2/gpcm with the revival servers' addresses,
 *      so CPSDnsIp (host-order IPv4, like CPSMyIp) is set to it. Default 178.62.43.212 (Poke Classic Network);
 *      config.txt key wifi_dns=<ipv4> overrides.
 * Nothing here touches the save. Log tag [NET]; every step is flushed to the SD card at once. */
#include <3ds.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern void PSPNativeMemLog(const char *, ...);
extern void P3DS_LogFlushSync(void);
extern int P3DS_CfgStr(const char *, char *, int);
extern unsigned long CPSDnsIp[2]; /* libntrwifi cps; u32 there, 4 bytes on this ABI */
#define NLOG(...) do { PSPNativeMemLog(__VA_ARGS__); P3DS_LogFlushSync(); } while (0)

#define DEFAULT_DNS "178.62.43.212"
#define SOC_BUF_SIZE 0x100000

static int sState; /* 0 untried, 1 sockets up, -1 failed */
static void *sSocBuf;

static void netExit(void)
{
    if (sState == 1) socExit();
    free(sSocBuf);
    sSocBuf = NULL;
    sState = 0;
}

int P3DS_NetInit(void)
{
    if (sState) return sState > 0;
    sState = -1;
    u32 wifi = 0;
    Result rc = acInit();
    if (R_SUCCEEDED(rc)) {
        rc = ACU_GetWifiStatus(&wifi);
        acExit();
    }
    NLOG("[NET] Wi-Fi status rc=0x%08lx status=%lu", (unsigned long)rc, (unsigned long)wifi);
    if (R_FAILED(rc) || wifi == 0) {
        NLOG("[NET] no Wi-Fi connection: online mode stays off");
        return 0;
    }
    sSocBuf = memalign(0x1000, SOC_BUF_SIZE);
    rc = sSocBuf ? socInit(sSocBuf, SOC_BUF_SIZE) : -1;
    NLOG("[NET] socInit rc=0x%08lx", (unsigned long)rc);
    if (R_FAILED(rc)) {
        free(sSocBuf);
        sSocBuf = NULL;
        return 0;
    }
    sState = 1;
    atexit(netExit);
    return 1;
}

/* struct layout must match toolchain-3ds/pspshim/ifaddrs.h (network.c is compiled with that header) */
struct ifaddrs {
    struct ifaddrs *ifa_next;
    char *ifa_name;
    unsigned int ifa_flags;
    struct sockaddr *ifa_addr;
    struct sockaddr *ifa_netmask;
};

static in_addr_t gDns; /* network order */

static void setDns(void)
{
    char dns[64];
    if (!P3DS_CfgStr("wifi_dns", dns, sizeof dns) || !dns[0]) strcpy(dns, DEFAULT_DNS);
    in_addr_t a = inet_addr(dns);
    if (a == (in_addr_t)-1) {
        NLOG("[NET] wifi_dns=%s is not an IPv4 address, using %s", dns, DEFAULT_DNS);
        a = inet_addr(DEFAULT_DNS);
    }
    gDns = a;
    CPSDnsIp[0] = CPSDnsIp[1] = ntohl(a);
    NLOG("[NET] DNS server %s", dns);
}

int getifaddrs(struct ifaddrs **ifap)
{
    static struct ifaddrs ifa;
    static struct sockaddr_in sa;
    static char name[] = "wlan0";
    *ifap = NULL;
    if (!P3DS_NetInit()) return 0;
    setDns();
    in_addr_t ip = 0;
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd >= 0) {
        struct sockaddr_in to, me;
        socklen_t len = sizeof me;
        memset(&to, 0, sizeof to);
        to.sin_family = AF_INET;
        to.sin_port = htons(53);
        to.sin_addr.s_addr = gDns;
        if (connect(fd, (struct sockaddr *)&to, sizeof to) == 0 && getsockname(fd, (struct sockaddr *)&me, &len) == 0) ip = me.sin_addr.s_addr;
        close(fd);
    }
    NLOG("[NET] local IP %u.%u.%u.%u", (unsigned)(ip & 255), (unsigned)((ip >> 8) & 255), (unsigned)((ip >> 16) & 255), (unsigned)(ip >> 24));
    if (!ip) return 0;
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = ip;
    ifa.ifa_next = NULL;
    ifa.ifa_name = name;
    ifa.ifa_addr = (struct sockaddr *)&sa;
    *ifap = &ifa;
    return 0;
}

void freeifaddrs(struct ifaddrs *list) { (void)list; }
