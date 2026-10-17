/* linkage_pair_main.c —— TU #0：通过 extern 使用别处定义的状态
 *
 * 链接方式（三个文件必须一起给链接器，见本目录 build.cmd）：
 *   gcc -std=c11 -Wall -Wextra -Wpedantic -g linkage_pair_main.c linkage_pair_a.c linkage_pair_b.c \
 *       -o ..\output\linkage_pair.exe
 * 运行：..\output\linkage_pair.exe
 * 【待实测】
 */
#include <stdio.h>

/* 别处定义的变量：声明（不是定义！） */
extern int g_shared;
extern int g_table[];

/* 别处定义的函数：原型。真实项目里这六行全在 .h 里 */
int  a_compute(void);
void a_bump(void);
int  b_compute(void);
int  b_table_sum(void);

int main(void)
{
    puts("== 跨 TU 共享状态 与 同名内部链接函数 ==");
    printf("  初始 g_shared    = %d   期望 100\n", g_shared);
    printf("  a_compute()      = %d   期望 141  (=100 + helper_seed_a=1 + 2*g_table[1]=40)\n", a_compute());
    printf("  b_compute()      = %d   期望 1100 (=100 + helper_seed_b=1000)\n", b_compute());
    printf("  a - b            = %d   期望 -959  <-- 两个 helper_seed 同名却是不同实体\n",
           a_compute() - b_compute());
    printf("  b_table_sum()    = %d   期望 100  (10+20+30+40，跨 TU 读数组)\n", b_table_sum());

    a_bump();
    printf("  a_bump() 后 g_shared=%d 期望 107;  a=%d 期望 148;  b=%d 期望 1107\n",
           g_shared, a_compute(), b_compute());

    g_shared = 1;                       /* main 也能改：证明它是同一个对象，不是每 TU 一份副本 */
    printf("  直接置 1 后：a=%d b=%d 期望 42 / 1001\n", a_compute(), b_compute());

    puts("");
    puts("== 动手实验（在 cmd 里逐条跑，观察链接器怎么说话）==");
    puts("  1) nm ..\\output\\linkage_pair_a.o   期望：D g_shared / T a_compute / t helper_seed");
    puts("  2) 删掉 a.c 中 int g_shared 的 static 之前先试：把它改成 static int g_shared");
    puts("     -> 重新链接：undefined reference to `g_shared'（外部链接没了）");
    puts("  3) 在 b.c 里加一行 int g_shared;");
    puts("     -> 重新链接：multiple definition of `g_shared'（GCC 10+ 默认 -fno-common）");
    puts("  4) 把 b.c 里 helper_seed 的 static 去掉");
    puts("     -> 重新链接：multiple definition of `helper_seed'");
    return 0;
}
