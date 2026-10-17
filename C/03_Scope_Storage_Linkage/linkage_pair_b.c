/* linkage_pair_b.c —— TU #2：同名 static 函数的另一份实现
 *
 * 本文件与 linkage_pair_a.c 都有一个叫 helper_seed 的函数。
 * 因为它们都是 static（内部链接），链接器把它们当成两个互不相干的符号 —— 不报 multiple definition。
 * 如果把这里的 static 去掉，立刻变成：multiple definition of `helper_seed'
 * 【待实测】
 */
#include <stddef.h>

/* 引用别处定义的符号：这里只是声明，不分配空间。
 * ⚠️ 真实项目应把这些 extern 声明放进一个 .h，由两个 .c 共同包含；
 *    这里故意写在本文件里，让"声明 vs 定义"的差别一眼可见。*/
extern int g_shared;                  /* 声明：不产生定义符号 */
extern int g_table[];                 /* ✅ 数组的正确外部声明：不完整数组类型，而不是 int* */
enum { G_TABLE_COUNT = 4 };           /* 长度必须由约定/头文件提供，跨 TU 拿不到 sizeof */

static int helper_seed(void)
{
    return 1000;                      /* b.c 的"种子"是 1000 —— 与 a.c 那份毫无关系 */
}

int b_compute(void);
int b_table_sum(void);

int b_compute(void)
{
    return g_shared + helper_seed();   /* g_shared=100 时 => 1100 */
}

int b_table_sum(void)
{
    int s = 0;
    for (size_t k = 0; k < G_TABLE_COUNT; ++k) {
        s += g_table[k];
    }
    return s;                          /* 10+20+30+40 = 100 */
}

/* 想亲手验证"不完整数组类型"的保护作用，把下一行取消注释：编译会报错。
   这正是它比 extern int *g_table; 好的地方 —— 后者能编译通过并在运行时读飞。
     size_t broken = sizeof g_table;
*/

