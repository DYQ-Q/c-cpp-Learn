/* pointer_arithmetic.c —— 指针缩放、数组退化、合法边界、半开区间
 *
 * 编译（本目录 build.cmd 会自动做同样的事）：
 *   gcc -std=c11 -Wall -Wextra -Wpedantic -g pointer_arithmetic.c -o ..\output\pointer_arithmetic.exe
 * 运行：
 *   ..\output\pointer_arithmetic.exe
 *
 * 设计原则：只打印**差值与大小**，不打印真实地址（ASLR 让每次地址都不同，无法核对）。
 * 若 %zu / %td 打印出 0 或怪值，追加 -D__USE_MINGW_ANSI_STDIO=1（见 指针与内存.md 第 1 节、
 * C语言基础语法.md 7.2）。【待实测】
 *
 * 预期输出（64 位 MinGW，sizeof(int*)==8, sizeof(int)==4, sizeof(double)==8）：
 *   第 1 节 步长：int 距离 4、char 距离 1、double 距离 8
 *   第 3 节 调用点 elems=5，函数内部 elems=2  <-- 8/4，这就是退化后的失真
 *   第 5 节 展平判定 yes（m 是连续内存）
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

enum { N = 5, ROWS = 2, COLS = 3 };

/* 在函数里，数组参数已经是指针：这里 sizeof a == sizeof(int*) 而不是 20。
 * clang 会用 -Wsizeof-array-argument 专门警告这一行；GCC 是否警告：待实测。 */
static void show_degenerated_size(const int a[N])
{
    printf("  [函数内部] sizeof a=%zu  elems=%zu   <-- 与真实元素数 %d 无关\n",
           sizeof a, sizeof a / sizeof a[0], N);
}

/* 正确的长度来源：只在"数组还是数组"的地方求 */
#define NELEM(a) (sizeof(a) / sizeof((a)[0]))

/* 半开区间 [begin, end) 扫描 —— p < end 合法，end 永不解引用 */
static long sum_range(const int *begin, const int *end)
{
    long s = 0;
    for (const int *p = begin; p < end; ++p) {
        s += *p;
    }
    return s;
}

/* 就地转换并返回 one-past-the-end，调用方可以接着往回写 */
static char *lower_copy(char *dst, const char *src, size_t len)
{
    char *d = dst;
    for (size_t i = 0; i < len; ++i) {
        char c = src[i];
        *d++ = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
    }
    *d = '\0';
    return d;
}

/* 用指针而不是下标写一遍 reverse，体现 p[i] == *(p+i) */
static void reverse_ints(int *first, int *last /* one-past-the-end */)
{
    for (--last; first < last; ++first, --last) {
        int t = *first;
        *first = *last;
        *last = t;
    }
}

int main(void)
{
    int    a[N] = { 10, 20, 30, 40, 50 };
    char   c[N] = { 0, 0, 0, 0, 0 };
    double d[N] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
    int    m[ROWS][COLS] = { { 1, 2, 3 }, { 4, 5, 6 } };

    puts("== 1. 算术步长由指针类型决定 ==");
    printf("  sizeof(void*)=%zu sizeof(int)=%zu sizeof(char)=%zu sizeof(double)=%zu\n",
           sizeof(void *), sizeof(int), sizeof(char), sizeof(double));
    printf("  int    相邻元素字节距离 = %zu\n",
           (size_t) ((uintptr_t) &a[1] - (uintptr_t) &a[0]));
    printf("  char   相邻元素字节距离 = %zu\n",
           (size_t) ((uintptr_t) &c[1] - (uintptr_t) &c[0]));
    printf("  double 相邻元素字节距离 = %zu\n",
           (size_t) ((uintptr_t) &d[1] - (uintptr_t) &d[0]));
    printf("  &a[1] - &a[0] (ptrdiff_t，单位是元素) = %td\n", (ptrdiff_t) (&a[1] - &a[0]));

    puts("== 2. one-past-the-end：可构造可比较，不可解引用 ==");
    int *end = a + N;
    printf("  end > end-1 ? %d      区间元素数 end-a = %td\n",
           end > end - 1, (ptrdiff_t) (end - a));
    printf("  sum_range(a, a+N) = %ld\n", sum_range(a, end));

    puts("== 3. 数组退化 ==");
    printf("  [调用点] sizeof a=%zu  NELEM(a)=%zu\n", sizeof a, NELEM(a));
    show_degenerated_size(a);
    int *p = a;
    printf("  p+2-p = %td   p[2]=%d   2[p]=%d（同一条表达式的两种写法）\n",
           (ptrdiff_t) ((p + 2) - p), p[2], 2[p]);

    puts("== 4. 指向数组的指针：步长里带长度 ==");
    int (*rp)[N] = &a;
    printf("  sizeof *rp = %zu   （等于整个数组 %zu）\n", sizeof *rp, sizeof a);
    printf("  (*rp)[2] = %d      rp[0][3] = %d\n", (*rp)[2], rp[0][3]);

    puts("== 5. 二维数组连续性与展平公式 ==");
    printf("  行步长（字节） = %zu  列数=%d  sizeof(*m)=%zu\n",
           (size_t) ((uintptr_t) &m[1][0] - (uintptr_t) &m[0][0]), COLS, sizeof *m);
    int flat_ok = 1;
    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            if (&m[i][j] != &m[0][0] + i * COLS + j) {
                flat_ok = 0;
            }
        }
    }
    printf("  &m[i][j] == &m[0][0] + i*COLS + j  全部成立? %s\n", flat_ok ? "yes" : "no");
    const int (*mc)[COLS] = m;              /* 列数进类型，所以能下标 */
    printf("  mc[1][2] = %d\n", mc[1][2]);

    puts("== 6. 能安全装下指针的整数类型 ==");
    printf("  sizeof(void*)=%zu sizeof(int)=%zu sizeof(long)=%zu sizeof(intptr_t)=%zu\n",
           sizeof(void *), sizeof(int), sizeof(long), sizeof(intptr_t));
    printf("  long 与指针同宽? %s  （Windows x86-64 是 LLP64：应为 no）\n",
           sizeof(long) == sizeof(void *) ? "yes" : "no");

    puts("== 7. 返回写入终点的惯用法 + 就地 reverse ==");
    char buf[32];
    char *stop = lower_copy(buf, "HeLLo World", 11);
    printf("  buf=\"%s\"  写入长度=%td  含终止符共=%td\n",
           buf, (ptrdiff_t) (stop - buf), (ptrdiff_t) (strlen(buf) + 1));
    reverse_ints(a, a + N);
    printf("  reverse 后：");
    for (int i = 0; i < N; ++i) {
        printf("%d ", a[i]);
    }
    puts("（期望 50 40 30 20 10）");

    return 0;
}
