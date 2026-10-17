/* dynamic_matrix.c —— 动态二维数组的三种布局、分配失败清理、访问顺序代价
 *
 * 编译：
 *   gcc -std=c11 -Wall -Wextra -Wpedantic -g dynamic_matrix.c -o ..\output\dynamic_matrix.exe
 * 想看到有意义的耗时差，再用优化版跑一遍：
 *   gcc -std=c11 -O2 dynamic_matrix.c -o ..\output\dynamic_matrix_O2.exe
 *
 * 本文件不依赖随机数、不读输入，输出**完全确定**（耗时行除外）。
 * g_live（已分配未释放的块数）用来证明失败清理路径没有泄漏 —— 没有 ASan 时的土办法。
 * 【待实测】
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>

enum { FIXED_COLS = 4 };
enum { BIG_R = 800, BIG_C = 800, REPEATS = 4 };

/* ---- 分配记账：把"泄漏"变成可打印的数字 ---- */
static size_t   g_budget = SIZE_MAX;   /* 允许成功的分配次数，用于确定性地模拟失败 */
static long long g_live   = 0;         /* 当前已分配未释放 */
static size_t    g_calls  = 0;         /* 累计尝试次数 */

static void *xmalloc(size_t n)
{
    ++g_calls;
    if (g_budget == 0) {
        return NULL;                   /* 模拟 OOM，走失败分支 */
    }
    --g_budget;
    void *p = malloc(n);
    if (p != NULL) {
        ++g_live;
    }
    return p;
}

static void xfree(void *p)
{
    if (p != NULL) {
        --g_live;
        free(p);
    }
}

/* ---- 布局 A：一维展平（默认选择：1 次分配、连续、缓存友好）---- */
static int *flat_new(size_t r, size_t c)
{
    int *a = xmalloc(r * c * sizeof *a);
    if (a == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < r * c; ++i) {
        a[i] = (int) i;
    }
    return a;
}

static void flat_free(int *a)
{
    xfree(a);                          /* 一次搞定 */
}

/* ---- 布局 B：行指针数组（r+1 次分配；必须处理"建到一半失败"）---- */
static int **jagged_new(size_t r, size_t c)
{
    int **m = xmalloc(r * sizeof *m);
    if (m == NULL) {
        return NULL;
    }
    size_t i = 0;
    for (; i < r; ++i) {
        m[i] = xmalloc(c * sizeof **m);
        if (m[i] == NULL) {
            goto fail;
        }
        for (size_t j = 0; j < c; ++j) {
            m[i][j] = (int) (i * c + j);
        }
    }
    return m;

fail:
    while (i > 0) {
        xfree(m[--i]);                 /* 只释放已成功分配的行；m[i] 从未被写入，不能碰 */
    }
    xfree(m);
    return NULL;
}

static void jagged_free(int **m, size_t r)
{
    if (m == NULL) {
        return;
    }
    for (size_t i = 0; i < r; ++i) {
        xfree(m[i]);
    }
    xfree(m);
}

/* ---- 布局 C：列数编译期已知的"动态行数二维数组"（1 次分配、连续、能用 m[i][j]）---- */
static int (*fixed_cols_new(size_t r))[FIXED_COLS]
{
    int (*m)[FIXED_COLS] = xmalloc(r * sizeof *m);   /* sizeof *m == FIXED_COLS*sizeof(int) */
    if (m == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < r; ++i) {
        for (size_t j = 0; j < FIXED_COLS; ++j) {
            m[i][j] = (int) (i * FIXED_COLS + j);
        }
    }
    return m;
}

static void fixed_cols_free(int (*m)[FIXED_COLS])
{
    xfree(m);
}

/* ---- 遍历耗时：同一块内存，行优先 vs 列优先 ---- */
static volatile long long g_sink;      /* 防止累加被优化器整段删掉 */

static double bench(const int *a, size_t r, size_t c, int column_major)
{
    clock_t t0 = clock();
    long long s = 0;
    for (int rep = 0; rep < REPEATS; ++rep) {
        if (column_major) {
            for (size_t j = 0; j < c; ++j) {
                for (size_t i = 0; i < r; ++i) {
                    s += a[i * c + j];  /* 每列跨 r*c 字节，缓存不友好 */
                }
            }
        } else {
            for (size_t i = 0; i < r; ++i) {
                for (size_t j = 0; j < c; ++j) {
                    s += a[i * c + j];  /* 顺序扫描，可预取 */
                }
            }
        }
    }
    g_sink = s;
    return (double) (clock() - t0) / (double) CLOCKS_PER_SEC;
}

static void report(const char *tag, size_t calls_before)
{
    printf("  %-26s 分配尝试=%zu  当前未释放块=%lld\n",
           tag, g_calls - calls_before, g_live);
}

int main(void)
{
    puts("== A. 一维展平 ==");
    size_t mark = g_calls;
    int *flat = flat_new(3, 5);
    report("flat_new(3,5)", mark);
    printf("  flat[2*5+1] = %d（等价于二维的 m[2][1]）\n", flat[2 * 5 + 1]);
    mark = g_calls;
    flat_free(flat);
    report("flat_free", mark);

    puts("== B. 行指针数组（正常路径）==");
    mark = g_calls;
    int **jag = jagged_new(3, 5);
    report("jagged_new(3,5)", mark);
    printf("  jag[2][1] = %d   顶层与各行的地址是否连续？自行比较即可\n", jag[2][1]);
    mark = g_calls;
    jagged_free(jag, 3);
    report("jagged_free", mark);

    puts("== C. 列数固定的动态行（m[i][j] 语法可用）==");
    mark = g_calls;
    int (*fc)[FIXED_COLS] = fixed_cols_new(3);
    report("fixed_cols_new(3)", mark);
    printf("  sizeof *fc = %zu（一行的字节数，即 fc+1 的步长）  fc[2][3] = %d\n",
           sizeof *fc, fc[2][3]);
    mark = g_calls;
    fixed_cols_free(fc);
    report("fixed_cols_free", mark);

    puts("== D. 模拟分配失败：验证清理路径不留垃圾 ==");
    mark = g_calls;
    g_budget = 3;                       /* 只够 m + row0 + row1 */
    int **bad = jagged_new(4, 5);
    g_budget = SIZE_MAX;
    printf("  budget=3 时 jagged_new(4,5) 返回 %s\n", bad == NULL ? "NULL（预期）" : "非空（异常）");
    report("失败清理后", mark);
    printf("  >>> g_live 必须为 0，否则就是泄漏（本行是这段测试的验收点）\n");

    mark = g_calls;
    g_budget = 0;                       /* 顶层就失败 */
    int **bad2 = jagged_new(4, 5);
    g_budget = SIZE_MAX;
    printf("  budget=0 时返回 %s，未释放块=%lld\n", bad2 == NULL ? "NULL（预期）" : "非空（异常）", g_live);
    report("顶层失败后", mark);

    puts("== E. 访问顺序的代价（-O2 下差距才明显）==");
    int *big = flat_new(BIG_R, BIG_C);
    if (big != NULL) {
        double row_first = bench(big, BIG_R, BIG_C, 0);
        double col_first = bench(big, BIG_R, BIG_C, 1);
        printf("  %dx%d x%d 次遍历：行优先 %.4fs   列优先 %.4fs   比值 %.1fx\n",
               BIG_R, BIG_C, REPEATS, row_first, col_first,
               row_first > 0.0 ? col_first / row_first : 0.0);
        printf("  （clock() 粒度约 1ms，若两者都接近 0，请改用 -O2 版本或加大 BIG_R）\n");
        flat_free(big);
    } else {
        puts("  大数组分配失败，跳过计时");
    }

    printf("== 收尾：未释放块=%lld（期望 0） 累计分配尝试=%zu ==\n", g_live, g_calls);
    return g_live == 0 ? 0 : 1;         /* 退出码即测试结果，便于脚本判断 */
}
