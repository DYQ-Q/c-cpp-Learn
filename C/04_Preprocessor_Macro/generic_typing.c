/* generic_typing.c —— C11 _Generic：让编译器按类型选函数
 *
 * 编译：gcc -std=c11 -Wall -Wextra -Wpedantic -g generic_typing.c -o ..\output\generic_typing.exe -lm
 * 【待实测】
 *
 * 三条会被本文件证明的性质：
 *   1) 控制表达式**不被求值**（所以 _Generic 天生没有宏的"多次求值"毛病）
 *   2) 数组会退化，因此数组类型分支永远匹配不到（本文件用 int (*)[3] 演示）
 *   3) 让 default 指向一个"参数个数不对"的函数，就能把"不支持的类型"变成编译错误
 */
#include <stdio.h>
#include <math.h>
#include <stdint.h>

/* ===== 1. 具体实现：真实函数，保留类型检查与调试信息 ===== */
static int    sq_i(int v)         { return v * v; }
static double sq_d(double v)      { return v * v; }
static float  sq_f(float v)       { return v * v; }
static long   sq_l(long v)        { return v * v; }

/* default 分支故意指向一个"零参数"函数：一旦被选中，编译报 "too many arguments"
   —— 这就是"编译期拒绝不支持的类型"的标准手法（而不是运行时打印 'unsupported type'）。
   另一个教训：_Generic 结果后面那对实参只写一次 `(x)`。写成 `(x), (x)` 会让 x 求值两次，
   如果 x 是 probe() 之类就有副作用 —— 分发本身安全，滥用宏参数照样翻车。 */
static int sq_unsupported(void);
static int sq_unsupported(void) { return -1; }

#define SQ(x) _Generic((x),                                  \
        int:     sq_i,                                       \
        long:    sq_l,                                       \
        float:   sq_f,                                       \
        double:  sq_d,                                       \
        default: sq_unsupported)(x)
/*  用法：SQ(a) 展开后就是 sq_i(a) —— 类型检查、调试符号、单步全都还在。 */

/* ===== 3. 按类型选择打印格式串 ===== */
#define SAY(v) printf(_Generic((v),                            \
        int:          "int    = %d\n",                        \
        unsigned int: "unsigned= %u\n",                       \
        long long:    "ll      = %lld\n",                     \
        double:       "double  = %.4f\n",                     \
        float:        "float   = %.4f\n",                     \
        char *:       "char*   = %s\n",                       \
        const char *: "const char* = %s\n",                   \
        default:      "other   = %p\n"), (v))
/*  C 里字符串字面量的类型是 char[N]（不是 const char[N]！那是 C++），
 *  所以 SAY("literal") 退化后匹配的是 char* 分支；const char* 分支要由"确实是 const 指针"
 *  的实参才会命中，例如 const char *p = "x"; SAY(p);
 *  另外不能写 printf(SAY_FMT(v) "\n", v)：宏展开结果不是字符串字面量，相邻拼接不成立。 */

static int g_probe_calls;
static int probe(void)
{
    ++g_probe_calls;
    return 7;
}

int main(void)
{
    puts("== 1. 按类型分发到不同函数 ==");
    int          a = 5;
    double       b = 1.5;
    float        c = 2.0f;
    long         d = 100L;
    printf("  SQ(int)    = %d（期望 25）\n", SQ(a));
    printf("  SQ(double) = %.1f（期望 2.25）\n", SQ(b));
    printf("  SQ(float)  = %.1f（期望 4.0）\n", (double) SQ(c));
    printf("  SQ(long)   = %ld（期望 10000）\n", SQ(d));

    puts("== 2. 控制表达式不被求值 ==");
    g_probe_calls = 0;
    const char *kind = _Generic((probe(), g_probe_calls += 100),   /* ← 整个控制表达式 */
                                int: "int", default: "other");
    printf("  控制表达式里有副作用，但 g_probe_calls=%d 期望 0；选中分支=%s\n",
           g_probe_calls, kind);
    /*  ↑ 逗号表达式的类型是 int，所以匹配 int 分支。求值为"只取类型"，一次都不执行。 */

    puts("== 3. 只有选中的分支会求值（副作用只跑一次）==");
    g_probe_calls = 0;
    int r = SQ(probe());
    printf("  SQ(probe()) = %d，probe 实际被调用 %d 次（期望 49 / 1 次）\n", r, g_probe_calls);
    puts("  原因：控制表达式 (probe()) 只求类型不求值；实参 (x) 只在选中分支里出现一次。");

    puts("== 4. 数组退化：只能用 int (*)[3] 匹配 ==");
    int arr[3] = { 1, 2, 3 };
    int (*pa)[3] = &arr;
    const char *k1 = _Generic((arr), int *: "arr 退化成 int*",
                              default: "其它");
    const char *k2 = _Generic((pa), int *: "int*",
                              int (*)[3]: "指向 int[3] 的指针",
                              default: "其它");
    printf("  _Generic((arr)) -> %s\n", k1);
    printf("  _Generic((pa))  -> %s\n", k2);
    /*  k1 永远是 "arr 退化成 int*"：写 int[3]: 分支是死代码（标准不允许函数类型，
        数组类型虽可写但匹配不到退化后的控制表达式）。 */

    puts("== 5. SAY：一个宏，按类型换格式串 ==");
    int    i5 = -3;
    unsigned u5 = 4000000000u;
    long long ll5 = 1234567890123LL;
    double  d5 = 3.14159;
    float   f5 = 2.5f;
    char    mut[] = "mutable";
    const char *ro = "const pointer";
    SAY(i5);
    SAY(u5);
    SAY(ll5);
    SAY(d5);
    SAY(f5);
    SAY(mut);              /* char[] 退化 -> char* 分支 */
    SAY("literal");        /* 同样是 char* 分支：C 的字面量类型是 char[8] */
    SAY(ro);               /* 这个才命中 const char* 分支 */

    puts("== 6. 想做编译期拒绝：default 指向签名不匹配的函数 ==");
    puts("  把 SQ(\"text\") 写进代码试试看：default 分支被选中 => 报 too many arguments。");
    puts("  这就是库作者常用的写法：错误发生在编译期，而不是运行时打出 'unsupported type'。");

    puts("== 7. 现实建议 ==");
    puts("  一个类型：写 static inline 函数。两个：还是写函数。");
    puts("  三四种同语义不同类型（sqrtf/sqrt/sqrtl 这类）：才值得 _Generic。");
    puts("  再往上（需要对任意 T 生成容器）：那是宏生成 + void* 的领域，见 12_Generic_Container。");
    return 0;
}
