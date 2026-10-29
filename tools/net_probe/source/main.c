/* net_probe: standalone libctru app (no game code, never touches a save). Online trading step 2.
   Checks that the 3DS can reach the community Wi-Fi Connection server over plain sockets and records what it answers.
   Sends only harmless requests (an unknown NAS action, GTS info/challenge GETs; no account is created, no Pokemon touched).
   Log: sdmc:/3ds/net_probe_log.txt ([NET] lines). Optional sdmc:/3ds/net_probe_ip.txt = server IPv4 (default Poke Classic Network). */
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SOC_BUF_SIZE 0x100000
#define DEFAULT_IP "178.62.43.212"

static FILE *g_log;

static void say(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void say(const char *fmt, ...)
{
    char line[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    printf("%s\n", line);
    if (g_log) { fprintf(g_log, "[NET] %s\n", line); fflush(g_log); fsync(fileno(g_log)); }
    gfxFlushBuffers();
    gfxSwapBuffers();
}

/* connect with a timeout; returns fd or -1 */
static int tcp_connect(const char *ip, int port, int timeout_ms)
{
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (fd < 0) { say("socket() failed errno=%d", errno); return -1; }
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = inet_addr(ip);
    int r = connect(fd, (struct sockaddr *)&sa, sizeof sa);
    if (r < 0 && errno != EINPROGRESS) { say("connect %s:%d failed errno=%d", ip, port, errno); close(fd); return -1; }
    struct pollfd p = { fd, POLLOUT, 0 };
    r = poll(&p, 1, timeout_ms);
    if (r <= 0 || (p.revents & (POLLERR | POLLHUP))) {
        say("connect %s:%d %s (revents=0x%x)", ip, port, r == 0 ? "timed out" : "failed", p.revents);
        close(fd);
        return -1;
    }
    return fd;
}

static void dump_reply(const char *buf, int n)
{
    char head[400];
    int k = n < (int)sizeof head - 1 ? n : (int)sizeof head - 1;
    for (int i = 0; i < k; i++) head[i] = (buf[i] >= 32 && buf[i] < 127) || buf[i] == '\n' ? buf[i] : '.';
    head[k] = 0;
    for (char *s = head; *s; s++) if (*s == '\n') *s = '|';
    say("reply %d bytes: %s", n, head);
}

/* one plain HTTP exchange; method POST when body != NULL */
static void http_test(const char *label, const char *ip, int port, const char *host, const char *path, const char *body)
{
    say("--- %s", label);
    int fd = tcp_connect(ip, port, 8000);
    if (fd < 0) return;
    char req[640];
    int len;
    if (body)
        len = snprintf(req, sizeof req, "POST %s HTTP/1.0\r\nHost: %s\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s", path, host, (int)strlen(body), body);
    else
        len = snprintf(req, sizeof req, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host);
    int sent = send(fd, req, len, 0);
    say("sent %d/%d bytes to %s:%d (Host %s)", sent, len, ip, port, host);
    static char buf[2048];
    int total = 0;
    for (int tries = 0; tries < 6 && total < (int)sizeof buf - 1; tries++) {
        struct pollfd p = { fd, POLLIN, 0 };
        int r = poll(&p, 1, 4000);
        if (r <= 0) break;
        int n = recv(fd, buf + total, sizeof buf - 1 - total, 0);
        if (n <= 0) break;
        total += n;
    }
    if (total > 0) dump_reply(buf, total); else say("no reply (timeout or closed)");
    close(fd);
}

static void port_test(const char *ip, int port)
{
    int fd = tcp_connect(ip, port, 5000);
    say("port %d: %s", port, fd >= 0 ? "OPEN" : "closed/unreachable");
    if (fd >= 0) close(fd);
}

int main(void)
{
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    g_log = fopen("sdmc:/3ds/net_probe_log.txt", "w");

    char ip[64] = DEFAULT_IP;
    FILE *f = fopen("sdmc:/3ds/net_probe_ip.txt", "r");
    if (f) { if (fscanf(f, "%63s", ip) != 1) strcpy(ip, DEFAULT_IP); fclose(f); }
    say("net_probe: server %s (log sdmc:/3ds/net_probe_log.txt)", ip);

    u32 wifi = 0;
    Result rc = acInit();
    say("acInit rc=0x%08lx", rc);
    if (R_SUCCEEDED(rc)) { rc = ACU_GetWifiStatus(&wifi); say("ACU_GetWifiStatus rc=0x%08lx status=%lu (0=none, 1=old-3DS-slot, 2=new)", rc, wifi); }

    void *socbuf = memalign(0x1000, SOC_BUF_SIZE);
    rc = socbuf ? socInit(socbuf, SOC_BUF_SIZE) : -1;
    say("socInit rc=0x%08lx", rc);

    if (R_SUCCEEDED(rc) && wifi != 0) {
        http_test("GTS info over HTTP (expects a short binary reply)", ip, 80, "gamestats2.gs.nintendowifi.net", "/pokemondpds/worldexchange/info.asp?pid=1", NULL);
        http_test("GTS challenge page over HTTP (expects a 32-byte token)", ip, 80, "gamestats2.gs.nintendowifi.net", "/pokemondpds/common/page.asp?pid=1", NULL);
        http_test("NAS over plain HTTP (unknown action, creates nothing)", ip, 80, "nas.nintendowifi.net", "/ac", "action=test");
        port_test(ip, 443);
        port_test(ip, 29900);
        port_test(ip, 29901);
    } else {
        say("skipped: no Wi-Fi connection or no sockets");
    }
    say("done. Press START to exit.");

    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;
        gspWaitForVBlank();
    }
    if (R_SUCCEEDED(rc)) socExit();
    free(socbuf);
    acExit();
    if (g_log) fclose(g_log);
    gfxExit();
    return 0;
}
