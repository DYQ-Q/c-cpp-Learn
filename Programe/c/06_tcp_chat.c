/* ==========================================================================
 * 06_tcp_chat.c —— C 可以直接做网络程序（不需要任何框架）
 *
 * 演示能力：
 *   socket / bind / listen / accept / connect / send / recv 这 7 个调用，
 *   加上 select() 单线程同时盯住几十个连接 —— 这就是所有聊天软件、
 *   HTTP 服务器、游戏服务器的最小内核。
 *
 * 编译：gcc -O2 -Wall -Wextra c\06_tcp_chat.c -o out\06.exe -lws2_32
 * 用法：
 *   终端 A:  out\06.exe server 9000
 *   终端 B:  out\06.exe client 127.0.0.1 9000 小明
 *   终端 C:  out\06.exe client 127.0.0.1 9000 小红
 *   连上后先输入自己的昵称（一行），之后每行都会广播给所有人。
 *   客户端输入 /quit 退出；服务器 Ctrl+C 退出。
 *   也可以测：在浏览器输入 http://127.0.0.1:9000 —— 你会看到服务器读到的原始请求行。
 * ========================================================================== */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CLIENTS 32

typedef struct { SOCKET fd; char name[32]; int named; } Client;

static void wsa_init(void)
{
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d) != 0) {
        fprintf(stderr, "WSAStartup 失败\n");
        exit(1);
    }
}
static void wsa_fail(const char *what)
{
    fprintf(stderr, "%s 失败, winsock 错误码=%d\n", what, WSAGetLastError());
}

static void broadcast(Client *cs, int *n, const char *msg, size_t len, SOCKET skip)
{
    for (int i = 0; i < *n; i++)
        if (cs[i].fd != skip) send(cs[i].fd, msg, (int)len, 0);
}

static void remove_client(Client *cs, int *n, int idx)
{
    closesocket(cs[idx].fd);
    cs[idx] = cs[*n - 1];              /* O(1) 删除：拿最后一个补位 */
    (*n)--;
}

/* ------------------------------ 服务器 ------------------------------ */
static int run_server(int port)
{
    wsa_init();

    SOCKET srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv == INVALID_SOCKET) { wsa_fail("socket"); return 1; }

    int on = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof on);

    struct sockaddr_in me;
    memset(&me, 0, sizeof me);
    me.sin_family      = AF_INET;
    me.sin_port        = htons((unsigned short)port);
    me.sin_addr.s_addr = htonl(INADDR_ANY);        /* 所有网卡；只想本机用就 INADDR_LOOPBACK */

    if (bind(srv, (struct sockaddr *)&me, sizeof me) == SOCKET_ERROR) { wsa_fail("bind"); return 1; }
    if (listen(srv, 8) == SOCKET_ERROR)           { wsa_fail("listen"); return 1; }
    printf("[server] 监听 0.0.0.0:%d —— Ctrl+C 退出\n", port);

    Client cs[MAX_CLIENTS];
    int n = 0;

    for (;;) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(srv, &rfds);
        SOCKET maxfd = srv;
        for (int i = 0; i < n; i++) {
            FD_SET(cs[i].fd, &rfds);
            if (cs[i].fd > maxfd) maxfd = cs[i].fd;
        }

        struct timeval tv = { 1, 0 };              /* 1 秒超时 -> 循环可以被打断 */
        int ready = select((int)maxfd + 1, &rfds, NULL, NULL, &tv);
        if (ready == SOCKET_ERROR) { wsa_fail("select"); break; }
        if (ready == 0) continue;                  /* 没事件，继续等 */

        /* (1) 有新连接 */
        if (FD_ISSET(srv, &rfds)) {
            struct sockaddr_in pe;
            int plen = (int)sizeof pe;
            SOCKET c = accept(srv, (struct sockaddr *)&pe, &plen);
            if (c != INVALID_SOCKET) {
                if (n >= MAX_CLIENTS) {
                    const char *full = "服务器已满\n";
                    send(c, full, strlen(full), 0);
                    closesocket(c);
                } else {
                    cs[n].fd = c; cs[n].named = 0; cs[n].name[0] = 0;
                    char ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &pe.sin_addr, ip, sizeof ip);
                    printf("[join] %s:%u  在线 %d 人\n", ip, (unsigned)ntohs(pe.sin_port), n + 1);
                    const char *hi = "输入昵称后回车（单行）:\n";
                    send(c, hi, strlen(hi), 0);
                    n++;
                }
            }
        }

        /* (2) 某个连接发来了数据 */
        for (int i = 0; i < n; ) {
            if (!FD_ISSET(cs[i].fd, &rfds)) { i++; continue; }
            char buf[512];
            int r = recv(cs[i].fd, buf, sizeof buf - 1, 0);
            if (r <= 0) {                          /* 0 = 对方关闭；<0 = 出错 */
                printf("[leave] %s 断开\n", cs[i].named ? cs[i].name : "(未命名)");
                char msg[128];
                int m = snprintf(msg, sizeof msg, "*** %s 离开了\n",
                                 cs[i].named ? cs[i].name : "(未命名)");
                broadcast(cs, &n, msg, (size_t)m, cs[i].fd);
                remove_client(cs, &n, i);           /* 补位后不要 i++，重新检查同一槽 */
                continue;
            }
            buf[r] = 0;
            /* 去掉 \r\n */
            for (int k = 0; k < r; k++) if (buf[k] == '\r' || buf[k] == '\n') { buf[k] = 0; break; }
            if (!buf[0]) { i++; continue; }

            if (!cs[i].named) {
                snprintf(cs[i].name, sizeof cs[i].name, "%.30s", buf);
                cs[i].named = 1;
                char msg[128];
                int m = snprintf(msg, sizeof msg, "*** %s 加入了\n", cs[i].name);
                broadcast(cs, &n, msg, (size_t)m, cs[i].fd);
                printf("[name] %s\n", cs[i].name);
            } else if (!strcmp(buf, "/quit")) {
                char msg[128];
                int m = snprintf(msg, sizeof msg, "*** %s 退出\n", cs[i].name);
                broadcast(cs, &n, msg, (size_t)m, cs[i].fd);
                remove_client(cs, &n, i);
            } else {
                char msg[640];
                int m = snprintf(msg, sizeof msg, "[%s] %s\n", cs[i].name, buf);
                send(cs[i].fd, msg, (size_t)m, 0);            /* 回显给自己 */
                broadcast(cs, &n, msg, (size_t)m, cs[i].fd);   /* 广播给别人 */
            }
            i++;
        }
    }
    for (int i = 0; i < n; i++) closesocket(cs[i].fd);
    closesocket(srv);
    WSACleanup();
    return 0;
}
/* ------------------------------ 客户端 ------------------------------ */
static DWORD WINAPI recv_thread(LPVOID arg)
{
    SOCKET fd = (SOCKET)(UINT_PTR)arg;
    char buf[512];
    for (;;) {
        int r = recv(fd, buf, sizeof buf - 1, 0);
        if (r <= 0) break;                 /* 服务器关了 -> 退出线程 */
        buf[r] = 0;
        fputs(buf, stdout);
        fflush(stdout);
    }
    printf("\n[已与服务器断开]\n");
    return 0;
}

static int run_client(const char *host, int port, const char *nick)
{
    wsa_init();

    /* getaddrinfo 同时支持 IP 和域名（它会去查 DNS）——真实程序都应该这样写 */
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    char portstr[16];
    snprintf(portstr, sizeof portstr, "%d", port);

    int ga = getaddrinfo(host, portstr, &hints, &res);
    if (ga != 0) { fprintf(stderr, "解析 %s:%d 失败: %s\n", host, port, gai_strerror(ga)); return 1; }

    SOCKET fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == INVALID_SOCKET) { freeaddrinfo(res); return 1; }

    printf("[client] 正在连接 %s:%d ...\n", host, port);
    if (connect(fd, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
        wsa_fail("connect（服务器起来了吗？）");
        freeaddrinfo(res);
        return 1;
    }
    freeaddrinfo(res);

    /* 一个线程专门负责收，主线程专门负责发 —— 单线程做不到「边听边说」 */
    HANDLE th = CreateThread(NULL, 0, recv_thread, (LPVOID)(UINT_PTR)fd, 0, NULL);

    char line[512], out[540];
    snprintf(out, sizeof out, "%s\n", nick);        /* 第一行 = 昵称 */
    send(fd, out, (int)strlen(out), 0);

    for (;;) {
        if (!fgets(line, sizeof line, stdin)) break;
        size_t l = strlen(line);
        while (l && (line[l - 1] == '\n' || line[l - 1] == '\r')) line[--l] = 0;
        if (l == 0) continue;
        if (!strcmp(line, "/quit")) break;
        int m = snprintf(out, sizeof out, "%s\n", line);
        if (send(fd, out, m, 0) == SOCKET_ERROR) { wsa_fail("send"); break; }
    }

    shutdown(fd, SD_SEND);                          /* 告诉对方「我说完了」 */
    Sleep(150);
    closesocket(fd);
    if (th) { WaitForSingleObject(th, 1000); CloseHandle(th); }
    WSACleanup();
    return 0;
}

static void usage(const char *self)
{
    printf("用法:\n  %s server [端口]\n  %s client [主机] [端口] [昵称]\n", self, self);
    printf("例:  %s server 9000\n     %s client 127.0.0.1 9000 小明\n", self, self);
}

int main(int argc, char **argv)
{
    if (argc < 2) { usage(argv[0]); return 1; }
    if (!strcmp(argv[1], "server")) return run_server(argc >= 3 ? atoi(argv[2]) : 9000);
    if (!strcmp(argv[1], "client")) {
        const char *host = argc >= 3 ? argv[2] : "127.0.0.1";
        int port         = argc >= 4 ? atoi(argv[3]) : 9000;
        const char *nick = argc >= 5 ? argv[4] : "guest";
        return run_client(host, port, nick);
    }
    usage(argv[0]);
    return 1;
}

