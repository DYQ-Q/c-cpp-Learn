/* promotion_and_overflow.c —— 整型提升、通常算术转换、溢出与截断的实测表
 *
 * 编译：gcc -std=c11 -Wall -Wextra -Wpedantic -g promotion_and_overflow.c -o ..\output\promotion_and_overflow.exe
 * 想看 UB 的真实后果（**不建议**，仅为理解）：
 *   gcc -std=c11 -O0 -DRUN_UB=1 promotion_and_overflow.c -o ..\output\p_ub.exe
 *
 * 本文件默认**不执行任何 UB**：所有"看起来绕回"的演示都用 unsigned（定义行为）
 * 或有符号的安全版本完成。UB 只出现在 #ifdef RUN_UB 里，需要你自己显式打开。
 * 【待实测】
 */
#include <stdio.h>
#include <limits.h>
#include <stdint.h>

enum { N = 5 };

/* ---------- 安全工具：这几个函数就是本文件的结论 ---------- */

/* 有符号加法的溢出预防：先看符号再看距离，绝不真的溢出 */
static int add_checked(int a, int b, int *out)
{
    if (b > 0 && a > INT_MAX - b) {
        return -1;                       /* 上溢 */
    }
    if (b < 0 && a < INT_MIN - b) {
        return -1;                       /* 下溢 */
    }
    *out = a + b;
    return 0;
}

/* abs 的安全版：INT_MIN 无法用 int 表示其绝对值 */
static long long abs_wide(int x)
{
    return x == INT_MIN ? -(long long) x : (x < 0 ? -(long long) x : (long long) x);
}

/* 倒序遍历的安全写法：n == 0 时一次都不进循环 */
static size_t reverse_visit(const int *a, size_t n)
{
    size_t visits = 0;
    for (size_t k = n; k-- > 0; ) {      /* ✅ 先比较后自减，无符号绕回也不会越界 */
        visits += (size_t) (a[k] != 0);
    }
    return visits;
}

/* 乘法溢出前的除法检查（分配内存前的标准做法） */
static int mul_would_overflow(size_t count, size_t elem, size_t *out)
{
    if (count != 0 && elem > SIZE_MAX / count) {
        return -1;
    }
    *out = count * elem;
    return 0;
}

int main(void)
{
    /* ===== 1. 平台类型体检（顺便完成 基础语法.md 的练习 P3）===== */
    puts("== 1. 类型宽度与范围 ==");
    printf("  sizeof: char=%zu short=%zu int=%zu long=%zu long long=%zu\n",
           sizeof(char), sizeof(short), sizeof(int), sizeof(long), sizeof(long long));
    printf("  sizeof: size_t=%zu ptrdiff_t=%zu void*=%zu int8_t=%zu int32_t=%zu int64_t=%zu\n",
           sizeof(size_t), sizeof(ptrdiff_t), sizeof(void *),
           sizeof(int8_t), sizeof(int32_t), sizeof(int64_t));
    printf("  CHAR_BIT=%d  INT_MIN=%d INT_MAX=%d  LONG_MIN=%ld LONG_MAX=%ld\n",
           CHAR_BIT, INT_MIN, INT_MAX, LONG_MIN, LONG_MAX);
    printf("  LLONG_MAX=%lld  SIZE_MAX=%zu\n", LLONG_MAX, SIZE_MAX);
#ifdef __CHAR_UNSIGNED__
    puts("  __CHAR_UNSIGNED__ 已定义 => 本机 char 是无符号的");
#else
    puts("  __CHAR_UNSIGNED__ 未定义 => 本机 char 通常是 signed（GCC/x86 默认）");
#endif
    printf("  实测：char c=(char)200 -> (int)c = %d   |   (unsigned char)c = %d\n",
           (int) (char) 200, (int) (unsigned char) 200);

    /* ===== 2. 整数提升 ===== */
    puts("== 2. 整数提升：窄类型进运算就变成 int ==");
    char x = 30, y = 30;
    printf("  sizeof(x+y) = %zu（不是 1；说明运算发生在 int 上）\n", sizeof(x + y));
    printf("  x+y = %d\n", x + y);
    unsigned char ua = 200, ub = 200;
    printf("  unsigned char 200*200 = %d（int 装得下，所以没有绕回成 144）\n", ua * ub);
    short s1 = 40000;
    printf("  short s1 = %d  <- 40000 超出 short 范围，是实现定义的截断\n", s1);
    char truncated = (char) (200 + 200);
    printf("  char = (char)400 -> %d（低 8 位 0x90，signed 下为负）\n", (int) truncated);

    /* ===== 3. 通常算术转换 ===== */
    puts("== 3. 有符号与无符号混算：无符号赢 ==");
    int      i = -1;
    unsigned u = 1u;
    printf("  int(-1) < unsigned(1)  =>  %d  （1=真 0=假；期望 0，因为 -1 被转成大正数）\n", i < u);
    long     lg = -1L;
    unsigned un = 1u;
    printf("  long(-1) < unsigned(1) =>  %d  （本平台 long 与 int 同宽 => 结果仍是 unsigned）\n", lg < un);
    size_t   n = 0;
    printf("  size_t n=0 -> n-1 = %zu  （绕回，不是 -1）\n", n - 1);
    printf("  危险循环 for(size_t k=n-1; k>=0; --k) 会%s\n", "永不结束（k>=0 恒真）");

    const int arr[N] = { 0, 3, 0, 5, 7 };
    printf("  reverse_visit(非零元素计数, n=5) = %zu（期望 3）\n", reverse_visit(arr, N));
    printf("  reverse_visit(n=0)              = %zu（期望 0，且不越界）\n", reverse_visit(arr, 0));

    /* ===== 4. 溢出：定义行为 vs UB ===== */
    puts("== 4. 溢出 ==");
    unsigned wu = 0u;
    printf("  unsigned 0 - 1u = %u  （✅ 定义：模 2^32 = %u）\n", wu - 1u, UINT_MAX);
    int out;
    printf("  add_checked(INT_MAX, 1) 返回 %d, out=%d  （期望 -1 表示拦截）\n",
           add_checked(INT_MAX, 1, &out), out);
    printf("  add_checked(INT_MAX-5, 3) 返回 %d, out=%d（期望 0：没溢出，结果 %d）\n",
           add_checked(INT_MAX - 5, 3, &out), out, INT_MAX - 2);
    printf("  abs_wide(INT_MIN) = %lld（期望 2147483648；注意它放不进 int）\n", abs_wide(INT_MIN));
    size_t bytes = 0;
    printf("  mul_would_overflow(SIZE_MAX, 8) 返回 %d（期望 -1：分配前的必查项）\n",
           mul_would_overflow(SIZE_MAX, sizeof(long long), &bytes));
    printf("  malloc 前的检查：if (count != 0 && elem > SIZE_MAX/count) 拒绝\n");

    puts("== 5. 除法与取模 ==");
    printf("  -7/2=%d  -7%%2=%d  7/-2=%d  7%%-2=%d   （向零截断，余数符号跟被除数）\n",
           -7 / 2, -7 % 2, 7 / -2, 7 % -2);
    printf("  恒等式 (a/b)*b + a%%b == a ：%d\n", (-7 / 2) * 2 + (-7 % 2) == -7);

    puts("== 6. 移位 ==");
    printf("  1u << 31 = %u（写成 1 << 31 才是 UB）\n", 1u << 31);
    int neg = -8;
    printf("  -8 >> 1 = %d  （有符号右移是实现定义；GCC/x86 是算术右移=保持符号）\n", neg >> 1);
    printf("  (unsigned)-8 >> 1 = %u  （无符号右移一定是逻辑右移，定义明确）\n",
           (unsigned) neg >> 1);

#ifdef RUN_UB
    /* ⚠️ 以下每一行都是 UB。结果不可依赖，可能被编译器删除，也可能直接异常终止。
     *    打开它的唯一目的：观察"UB 不崩"这一事实，从而理解为什么它危险。 */
    puts("== 7. RUN_UB 已开启：演示 UB 的两种'看起来没事' ==");
    volatile int vmx = INT_MAX;
    int wrapped = vmx + 1;
    printf("  [UB] INT_MAX+1 = %d  （观察：多半是 %d，但没有承诺）\n", wrapped, INT_MIN);
    volatile int vmn = INT_MIN;
    int d = vmn / -1;
    printf("  [UB] INT_MIN/-1 = %d  （x86 上此处可能直接 SIGFPE/异常终止）\n", d);
    volatile int vzero = 0;
    printf("  [UB] 1/0 = %d\n", d / vzero);
#endif

    puts("== 结论 ==");
    puts("  1) 窄类型运算时是 int；赋值回去才截断");
    puts("  2) 有符号与无符号混算 -> 无符号赢，比较会反转；size_t 减法会绕回");
    puts("  3) 有符号溢出/除零 = UB；无符号回绕 = 定义。溢出必须先检查再运算");
    puts("  4) 溢出检查 if (x+1 < x) 会被优化删除 -> 用减法比较或升级到更宽类型");
    return 0;
}
