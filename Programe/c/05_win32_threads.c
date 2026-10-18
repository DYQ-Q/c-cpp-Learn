/* ==========================================================================
 * 05_win32_threads.c —— 直接调操作系统 API 做并发
 *
 * 演示能力：
 *   1. CreateThread / WaitForMultipleObjects：真正的多线程，不是「看起来并行」
 *   2. 竞态条件（race condition）现场重现：普通 ++ 与 InterlockedIncrement 对比
 *   3. CRITICAL_SECTION 保护共享输出，避免两个线程把 printf 打成乱码
 *   4. QueryPerformanceCounter 做微秒级计时，量出加速比
 *   5. 任务切分（把 1..N 的素数统计拆给 K 个线程）
 *
 * 编译：gcc -std=c17 -Wall -Wextra -O2 c\05_win32_threads.c -o out\05.exe
 * 运行：out\05.exe
 * 说明：用了 Win32 API（跨平台的等价物是 C11 threads.h / std::thread）。
 * ========================================================================== */
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static volatile LONG g_naive  = 0;      /* 故意用非原子 ++，用来翻车 */
static volatile LONG g_atomic = 0;      /* 原子累加，永远正确 */
static CRITICAL_SECTION g_log;

static void logf(const char *fmt, const char *who)
{
    EnterCriticalSection(&g_log);       /* 锁住整段输出，保证一行不被打断 */
    printf(fmt, who);
    fflush(stdout);
    LeaveCriticalSection(&g_log);
}

static double now_ms(void)
{
    static LARGE_INTEGER freq;
    static int inited = 0;
    if (!inited) { QueryPerformanceFrequency(&freq); inited = 1; }
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart * 1000.0 / (double)freq.QuadPart;
}

/* ---------- 1. 竞态条件：看得见摸得着的 bug ---------- */
#define RACY_ITERS 2000000
static DWORD WINAPI racer(LPVOID arg)
{
    (void)arg;
    for (int i = 0; i < RACY_ITERS; i++) {
        g_naive++;                          /* 编译成 读 / 加 / 写 三步，中间会被切走 */
        InterlockedIncrement(&g_atomic);    /* CPU 级的「读-改-写」原子指令 */
    }
    return 0;
}

static void demo_race(int nthreads)
{
    puts("===== 1. 竞态条件：同一个循环，两种写法 =====");
    g_naive = g_atomic = 0;
    HANDLE *h = calloc((size_t)nthreads, sizeof *h);
    for (int i = 0; i < nthreads; i++)
        h[i] = CreateThread(NULL, 0, racer, NULL, 0, NULL);
    WaitForMultipleObjects((DWORD)nthreads, h, TRUE, INFINITE);
    for (int i = 0; i < nthreads; i++) CloseHandle(h[i]);
    free(h);

    LONG expect = (LONG)nthreads * RACY_ITERS;
    printf("理论值        = %d\n", expect);
    printf("普通 ++       = %d   %s\n", (int)g_naive,
           g_naive == expect ? "（这次侥幸对了，多跑几遍就露馅）"
                             : " <-- 错了！线程互相踩了中间状态");
    printf("Interlocked   = %d   永远正确\n", (int)g_atomic);
    puts("这就是为什么并发程序必须显式同步：编译器不会帮你，CPU 也不一定帮你。\n");
}

/* ---------- 2. 任务切分 + 计时：并行到底快多少 ---------- */
static int is_prime(uint64_t n)
{
    if (n < 2) return 0;
    if ((n & 1) == 0) return n == 2;
    for (uint64_t d = 3; d * d <= n; d += 2)
        if (n % d == 0) return 0;
    return 1;
}

typedef struct { uint64_t lo, hi; LONG result; } Job;

static DWORD WINAPI count_primes(LPVOID arg)
{
    Job *j = arg;
    LONG c = 0;
    for (uint64_t n = j->lo; n < j->hi; n++) c += is_prime(n);
    j->result = c;                       /* 各自写各自的槽位 -> 天然无竞争 */
    return 0;
}

static void bench(uint64_t N, int nthreads)
{
    Job   *jobs = calloc((size_t)nthreads, sizeof *jobs);
    HANDLE *h   = calloc((size_t)nthreads, sizeof *h);
    LONG    sum = 0;

    double t0 = now_ms();
    for (int i = 0; i < nthreads; i++) {
        jobs[i].lo = 2 + (N - 2) * (uint64_t)i / (uint64_t)nthreads;
        jobs[i].hi = 2 + (N - 2) * (uint64_t)(i + 1) / (uint64_t)nthreads;
        jobs[i].result = 0;
    }
    for (int i = 0; i < nthreads; i++)
        h[i] = CreateThread(NULL, 0, count_primes, &jobs[i], 0, NULL);
    WaitForMultipleObjects((DWORD)nthreads, h, TRUE, INFINITE);
    for (int i = 0; i < nthreads; i++) { CloseHandle(h[i]); sum += jobs[i].result; }
    double ms = now_ms() - t0;

    free(jobs); free(h);
    printf("  %d 线程: %ld 个素数, 用时 %.1f ms\n", nthreads, sum, ms);
}

static void demo_parallel(void)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    printf("===== 2. 并行统计素数（本机 %lu 个逻辑核心）=====\n", si.dwNumberOfProcessors);
    const uint64_t N = 200000;
    printf("统计 2..%llu 的素数个数\n", (unsigned long long)N);
    int ks[4] = { 1, 2, 4, 8 };
    for (int i = 0; i < 4; i++) bench(N, ks[i]);
    puts("  注意：加速比不是线性的 —— 线程创建、内存带宽、核心数都是成本。\n");
}

/* ---------- 3. 不加锁 vs 加锁：输出乱成什么样 ---------- */
static DWORD WINAPI shout(LPVOID arg)
{
    const char *who = (const char *)arg;
    for (int i = 0; i < 3; i++) {
        char line[64];
        snprintf(line, sizeof line, "[%s] 线程自己的一段话\n", who);
        fputs(line, stdout);              /* 故意不加锁：可能与其他线程交错 */
        Sleep(1);
    }
    return 0;
}

static void demo_locking(void)
{
    puts("===== 3. 共享资源：控制台输出也算共享资源 =====");
    const char *names[3] = { "A", "B", "C" };
    HANDLE h[3];
    for (int i = 0; i < 3; i++) h[i] = CreateThread(NULL, 0, shout, (LPVOID)names[i], 0, NULL);
    WaitForMultipleObjects(3, h, TRUE, INFINITE);
    for (int i = 0; i < 3; i++) CloseHandle(h[i]);
    puts("  上面的行如果交错/断裂，就是没有同步的结果；logf() 那种写法用 CRITICAL_SECTION 修好它。\n");
}

int main(void)
{
    InitializeCriticalSection(&g_log);
    demo_race(4);
    demo_locking();
    demo_parallel();
    DeleteCriticalSection(&g_log);
    puts("结论：C 里「并行」是操作系统给你的能力，代价是你要自己保证数据不被踩坏。");
    return 0;
}
