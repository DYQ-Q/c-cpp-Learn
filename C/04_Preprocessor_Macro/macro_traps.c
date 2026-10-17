/* macro_traps.c —— 宏的六类坑与对应的正确形状
 *
 * 编译：gcc -std=c11 -Wall -Wextra -Wpedantic -g macro_traps.c -o ..\output\macro_traps.exe
 * 发布版对照（观察 assert 里的副作用被删掉）：
 *   gcc -std=c11 -DNDEBUG macro_traps.c -o ..\output\macro_traps_rel.exe
 * 看展开：gcc -std=c11 -E -P macro_traps.c | more
 * 【待实测】
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

/* ===== 1. 优先级：括号两处都要 ===== */
#define F_NAIVE(x)   x + 1
#define F_PAREN(x)   ((x) + 1)
#define MUL_NAIVE(x) x * 2
#define MUL_PAREN(x) ((x) * 2)

/* ===== 2. do{...}while(0) ===== */
#define BAD_LOG(m)   { puts(m); }
#define OK_LOG(m)    do { puts(m); } while (0)

/* ===== 3. # 字符串化与两级展开 ===== */
#define MAX_LEVEL    100
#define STR_(x)      #x
#define STR(x)       STR_(x)          /* 先展开 x，再字符串化 */
#define CAT_(a, b)   a##b
#define CAT(a, b)    CAT_(a, b)       /* 先展开参数，再拼接 */
#define LEVEL_1      "low"
#define LEVEL_2      "high"
#define PICK(n)      CAT(LEVEL_, n)   /* PICK(2) -> LEVEL_2 -> "high" */
/*  如果写成 CAT_(LEVEL_, TWO) 而 #define TWO 2，得到的是 LEVEL_TWO（不存在）=> 编译错误。
 *  这就是"## 的参数不做替换"，也是必须有两层宏的原因。*/

/* ===== 4. 变参日志：交给真函数，避开 __VA_ARGS__ 为空的老问题 ===== */
#if defined(__GNUC__)
#  define PRINTF_LIKE(f, first) __attribute__((format(printf, f, first)))
#else
#  define PRINTF_LIKE(f, first)
#endif

static int g_log_lines;

static void log_msg(const char *file, int line, const char *fmt, ...) PRINTF_LIKE(3, 4);

static void log_msg(const char *file, int line, const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n < 0) {
        return;
    }
    if ((size_t) n >= sizeof buf) {
        /* 尾部覆盖，标记被截断。绝不能用 snprintf(buf,...,"%s",buf) —— 源与目标重叠是 UB。 */
        static const char tail[] = "...[truncated]";
        memcpy(buf + (sizeof buf - sizeof tail), tail, sizeof tail);   /* 含结尾 '\0' */
    }
    printf("  [%s:%d] %s\n", file, line, buf);
    ++g_log_lines;
}

#define LOGC(...) log_msg(__FILE__, __LINE__, __VA_ARGS__)

/* 直接展开版的两种写法都在这里"备用"，注释掉是为了让本文件在 -std=c11 下干净编译：
 *   A) GNU 扩展：空实参时 ## 吃掉前面的逗号（-Wpedantic 会警告"非标准"）
 *      #define LOGG(fmt, ...) do { printf("  [gnu] " fmt "\n", ##__VA_ARGS__); } while (0)
 *   B) C23/C++20 标准写法：__VA_OPT__（-std=c23 起可用；C11 下 GCC 不认它）
 *      #define LOGS(fmt, ...) do { printf("  [c23] " fmt "\n" __VA_OPT__(,) __VA_ARGS__); } while (0)
 * 结论：可移植代码就用下面的 LOGC（把变参原封不动交给一个真函数），
 *       它天然没有"空实参留下尾随逗号"的问题，还能享受 format 检查。 */

/* ===== 5. assert 与副作用 ===== */
static int bump(int *p)
{
    ++*p;
    return 1;
}

/* ===== 6. 局部关闭警告的正确形状 ===== */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static void demo_suppress(void)
{
    int intentionally_unused = 1;    /* 不报警告：只在最小范围内关掉 */
    (void) intentionally_unused;
}
#pragma GCC diagnostic pop

static inline int min_i(int a, int b) { return a < b ? a : b; }   /* 宏的正确替代品 */

int main(void)
{
    puts("== 1. 优先级 ==");
    printf("  F_NAIVE(2)*3   = %d   期望 5（2+1*3，不是 9）\n", F_NAIVE(2) * 3);
    printf("  2*F_NAIVE(3)   = %d   期望 7（2*3+1，不是 8）\n", 2 * F_NAIVE(3));
    printf("  F_PAREN(2)*3   = %d   期望 9\n", F_PAREN(2) * 3);
    printf("  MUL_NAIVE(3+1) = %d   期望 7（3+1*2）\n", MUL_NAIVE(3 + 1));
    printf("  MUL_PAREN(3+1) = %d   期望 8\n", MUL_PAREN(3 + 1));

    puts("== 2. do/while(0) 保住 if-else ==");
    int cond = 0;
    if (cond) OK_LOG("never");
    else OK_LOG("else 分支正常：宏展开是一条语句");
    if (cond) OK_LOG("never");
    else OK_LOG("两个 do/while(0) 串起来也没问题");
    /*  对照实验：把第二行的 OK_LOG 换成 BAD_LOG(m) { puts(m); } 形态再编译，
        会得到 "expected expression before 'else'" 之类的错误 ——
        因为展开后是 if (cond) { puts("never"); };  else ... ，那个分号让 else 挂空。 */

    puts("== 3. # 与 ## ==");
    printf("  STR(MAX_LEVEL) = \"%s\"   （没有两级 => \"MAX_LEVEL\"）\n", STR(MAX_LEVEL));
    printf("  STR_(MAX_LEVEL)= \"%s\"   （直接字符串化 => 令牌文本）\n", STR_(MAX_LEVEL));
    printf("  STR(1 + 2)     = \"%s\"   （字符串化不计算！）\n", STR(1 + 2));
    printf("  PICK(1) = %s   PICK(2) = %s   （## 生成标识符再取它的值）\n", PICK(1), PICK(2));

    puts("== 4. 变参日志 ==");
    LOGC("hello");                                 /* 无变参：走函数，没有尾随逗号问题 */
    LOGC("n=%d size=%zu", 42, sizeof(int));
    g_log_lines = 0;
    LOGC("line counted");
    printf("  日志计数 g_log_lines = %d（期望 1）\n", g_log_lines);

    puts("== 5. assert 的副作用陷阱 ==");
    int plain = 0;
    bump(&plain); bump(&plain);
    int in_assert = 0;
    assert(bump(&in_assert) == 1);
    printf("  正常调用两次 = %d；放进 assert 一次 = %d\n", plain, in_assert);
    printf("  ==> 用 -DNDEBUG 重新编译，第二个数会变成 0：那次 bump 根本没跑。\n");
    assert(plain == 2 && "plain 应该被加过两次");

    puts("== 6. 宏 vs static inline ==");
    printf("  min_i(3, -1) = %d（有类型检查；传指针会直接编译报错，宏则照收）\n", min_i(3, -1));
    int i = 3;
    printf("  注意：SQUARE(i++) 会展开成 ((i++)*(i++)) => UB，本文件不执行它。\n");
    printf("  现在把 i 安全地平方：i=%d i*i=%d\n", i, i * i);

    demo_suppress();
    puts("== 7. 展开自检命令 ==");
    puts("  gcc -std=c11 -E -P macro_traps.c | more          （看真实展开）");
    puts("  gcc -std=c11 -ftrack-macro-expansion=2 -c macro_traps.c  （宏内报错的展开链）");
    return 0;
}
