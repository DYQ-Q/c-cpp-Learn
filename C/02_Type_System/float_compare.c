/* float_compare.c —— IEEE-754 行为实测：表示误差、累加、NaN/Inf、round-trip
 *
 * 编译：gcc -std=c11 -Wall -Wextra -Wpedantic -g float_compare.c -o ..\output\float_compare.exe -lm
 * 对照优化版（观察 -O2 / -ffast-math 造成的差异，这是本文件存在的另一半意义）：
 *   gcc -std=c11 -O2 float_compare.c -o ..\output\float_compare_O2.exe -lm
 *   gcc -std=c11 -O2 -ffast-math float_compare.c -o ..\output\float_compare_fast.exe -lm
 *
 * 本文件不含 UB：不对零做除法、不用 union 双关、不依赖未初始化值。
 * 【待实测】
 */
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>

enum { MANY = 100000 };

/* 相对 + 绝对容差：工程里真正能用的浮点比较 */
static int near_eq(double a, double b, double rel, double abs_tol)
{
    double d = fabs(a - b);
    double m = fmax(fabs(a), fabs(b));
    return d <= abs_tol || d <= rel * m;
}

static double naive_sum_d(const double *v, size_t n)
{
    double s = 0.0;
    for (size_t i = 0; i < n; ++i) {
        s += v[i];
    }
    return s;
}

/* Kahan 补偿求和：把每轮被丢掉的小量攒进 comp，下一轮减回来 */
static double kahan_sum_d(const double *v, size_t n)
{
    double sum = 0.0, comp = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double y = v[i] - comp;
        double t = sum + y;
        comp = (t - sum) - y;
        sum = t;
    }
    return sum;
}

static float naive_sum_f(const float *v, size_t n)
{
    float s = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        s += v[i];
    }
    return s;
}

static void roundtrip_probe(double v)
{
    char buf[64];
    int ok15, ok16, ok17, okhex;

    snprintf(buf, sizeof buf, "%.15g", v);
    ok15 = (strtod(buf, NULL) == v);
    snprintf(buf, sizeof buf, "%.16g", v);
    ok16 = (strtod(buf, NULL) == v);
    snprintf(buf, sizeof buf, "%.17g", v);
    ok17 = (strtod(buf, NULL) == v);
    snprintf(buf, sizeof buf, "%a", v);
    okhex = (strtod(buf, NULL) == v);

    printf("  %-.17g  15位:%s  16位:%s  17位:%s  %%a:%s\n",
           v, ok15 ? "ok" : "FAIL", ok16 ? "ok" : "FAIL",
           ok17 ? "ok" : "FAIL", okhex ? "ok" : "FAIL");
}

int main(void)
{
    puts("== 1. 本机浮点参数 ==");
    printf("  sizeof: float=%zu double=%zu long double=%zu\n",
           sizeof(float), sizeof(double), sizeof(long double));
    printf("  FLT_MANT_DIG=%d DBL_MANT_DIG=%d LDBL_MANT_DIG=%d  FLT_RADIX=%d\n",
           FLT_MANT_DIG, DBL_MANT_DIG, LDBL_MANT_DIG, FLT_RADIX);
    printf("  FLT_EVAL_METHOD=%d DBL_EVAL_METHOD=%d  （0=按各自类型算；2=中间量用 long double，32 位 x87 常见）\n",
           FLT_EVAL_METHOD, DBL_EVAL_METHOD);
    printf("  DBL_DECIMAL_DIG=%d  FLT_DIG=%d DBL_DIG=%d  （打印位数 >= DBL_DECIMAL_DIG 才能无损往返）\n",
           DBL_DECIMAL_DIG, FLT_DIG, DBL_DIG);
    printf("  DBL_EPSILON=%.20g\n", DBL_EPSILON);
    printf("  DBL_MIN=%.20g  DBL_TRUE_MIN=%.20g  DBL_MAX=%.20g\n",
           DBL_MIN, DBL_TRUE_MIN, DBL_MAX);
    puts("  提醒：DBL_EPSILON 是 1.0 附近的间距，不是'最小正数'。");

    puts("== 2. 0.1 从来不是 0.1 ==");
    printf("  0.1 的真实值   %.20g\n", 0.1);
    printf("  0.1 的十六进制 %a   <-- 循环节 999... 说明它是无限二进制小数\n", 0.1);
    printf("  0.1+0.2 的值   %.20g\n", 0.1 + 0.2);
    printf("  0.1+0.2 == 0.3 ? %d   （期望 0：别用 == 判浮点）\n", 0.1 + 0.2 == 0.3);
    printf("  两者十六进制： %a  vs  %a\n", 0.1 + 0.2, 0.3);

    puts("== 3. 整数精确边界：float 是 2^24，double 是 2^53 ==");
    {
        volatile float f24 = 16777216.0f;   /* 2^24 */
        volatile float f23 = 8388608.0f;    /* 2^23 */
        printf("  2^24 + 1 == 2^24 ? %d   （期望 1：+1 被吞）\n", f24 + 1.0f == f24);
        printf("  2^23 + 1 == 2^23 ? %d   （期望 0：这个量级还能表示相邻整数）\n", f23 + 1.0f == f23);
        printf("  (float)16777217 会变成 %g（期望 16777216）\n", (double) 16777217.0f);
        volatile double big = 1e16;
        printf("  1e16 + 1 == 1e16 ? %d   （double 同样有边界，在 2^53）\n", big + 1.0 == big);
    }

    puts("== 4. 累加误差与 Kahan 补偿 ==");
    {
        static double vd[MANY];
        static float  vf[MANY];
        for (int i = 0; i < MANY; ++i) {
            vd[i] = 0.1;
            vf[i] = 0.1f;
        }
        const double want = 10000.0;
        double naive  = naive_sum_d(vd, MANY);
        double kahan  = kahan_sum_d(vd, MANY);
        float  naivef = naive_sum_f(vf, MANY);
        printf("  把 0.1 连加 %d 次，期望 %.1f\n", MANY, want);
        printf("  double 朴素  = %.17g   误差 %.3e\n", naive, fabs(naive - want));
        printf("  double Kahan = %.17g   误差 %.3e\n", kahan, fabs(kahan - want));
        printf("  float  朴素  = %.9g    误差 %.3e   <-- 大批量累加用 float 是灾难\n",
               (double) naivef, fabs((double) naivef - want));
        puts("  若三项误差都是 0.000e+00：说明被常量折叠/向量化改了路径，用 -O0 与 -O2 各跑一遍对比。");
    }

    puts("== 5. NaN / Inf / 带符号的 0 ==");
    {
        double nanv = NAN;
        double infv = INFINITY;
        printf("  isnan(NAN)=%d   NaN==NaN ?%d   NaN!=NaN ?%d\n",
               (int) isnan(nanv), nanv == nanv, nanv != nanv);
        printf("  NaN<1.0 ?%d  NaN>1.0 ?%d  ==> 所有比较为假，'非小于非大于'不等于'等于'\n",
               nanv < 1.0, nanv > 1.0);
        printf("  isfinite(INF)=%d  isinf(INF)=%d  isfinite(1.0)=%d\n",
               (int) isfinite(infv), (int) isinf(infv), (int) isfinite(1.0));
        printf("  0.0 == -0.0 ?%d  但 signbit(-0.0)=%d  ==> 相等不代表同一个值\n",
               0.0 == -0.0, (int) signbit(-0.0));
        puts("  注：1.0/0.0 在 IEEE 实现里给出 INF，但 C 标准把浮点除零也列为 UB —— 不要写。");
        puts("     需要 Inf/NaN 就用 INFINITY / NAN 宏，读入数据后用 isfinite 检查。");
    }

    puts("== 6. 打印宽度与十进制往返 ==");
    puts("  值                     15/16/17 位有效数字与 %a 能否还原成同一个 double");
    roundtrip_probe(0.1);
    roundtrip_probe(1.0 / 3.0);
    roundtrip_probe(1234567890123456789.0);
    roundtrip_probe(DBL_MAX);
    roundtrip_probe(DBL_TRUE_MIN);
    puts("  结论：存档/跨进程传值用 %.17g（DBL_DECIMAL_DIG 位）或 %a；默认 %f 的 6 位会丢信息。");

    puts("== 7. 容差比较的两种失败模式 ==");
    {
        struct { double a, b, rel, abs_tol; } cases[] = {
            { 0.1 + 0.2,   0.3,        1e-9, 1e-12 },   /* 期望判为相等 —— 正确用法 */
            { 1e16,        1e16 + 2.0, 1e-9, 1e-12 },   /* 相对容差在此量级 = 1e7，宽到离谱 */
            { 1e-300,      2e-300,     1e-9, 1e-12 },   /* 绝对容差把极小值一律判成相等 */
        };
        for (size_t k = 0; k < sizeof cases / sizeof cases[0]; ++k) {
            double a = cases[k].a, b = cases[k].b;
            printf("  a=%-12.6g b=%-12.6g  a==b?%d  near_eq?%d\n",
                   a, b, a == b, near_eq(a, b, cases[k].rel, cases[k].abs_tol));
        }
        puts("  ==> 容差必须由业务量级决定（几何、金融、校验和各不同）。抄一个 1e-9 就交差是错的。");
    }

    puts("== 8. 本文件的验收点 ==");
    puts("  2 节：0.1+0.2 == 0.3 必须打印 0");
    puts("  3 节：2^24+1 == 2^24 必须打印 1");
    puts("  5 节：NaN==NaN 打印 0，NaN!=NaN 打印 1");
    puts("  4/6 节的具体数值随优化级别变化 —— 记录下来写回 md 第 7 节，把【待实测】改成【已验证】");

    return 0;
}
