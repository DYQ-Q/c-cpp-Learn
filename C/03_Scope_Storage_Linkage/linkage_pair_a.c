/* linkage_pair_a.c —— TU #1：外部链接的定义 + 内部链接的同名 helper
 *
 * 与 linkage_pair_b.c / linkage_pair_main.c 一起链接才能成 exe：
 *   gcc -std=c11 -Wall -Wextra -Wpedantic -g linkage_pair_main.c linkage_pair_a.c linkage_pair_b.c \
 *       -o ..\output\linkage_pair.exe
 *
 * 想亲手复现两个经典错误：
 *   A) multiple definition：在 b.c 里加一行 `int g_shared;`（tentative definition），
 *      GCC 15 默认 -fno-common => 报 multiple definition of `g_shared'。
 *   B) undefined reference：把 a.c 里 `int g_shared = 100;` 改成 `static int g_shared = 100;`
 *      => main/b 的 extern 声明找不到外部符号。
 * 【待实测】
 */

/* ===== 外部链接：全程序唯一的定义（声明在 main.c/b.c 里用 extern 完成） ===== */
int g_shared = 100;            /* 定义 + 初始化器 => 确定生成符号（nm 里是大写 D）*/
int g_table[4] = { 10, 20, 30, 40 };   /* 数组定义在这里；别的 TU 必须用 extern int g_table[]; */

/* ===== 内部链接：与 b.c 里的同名函数完全无关，链接器不报错 ===== */
static int helper_seed(void)
{
    return 1;                  /* a.c 的"种子"是 1 */
}

/* 只在本 TU 使用的私有函数 —— 应该永远是 static，不给外面看见的机会 */
static int scale_by_2(int v)
{
    return v * 2;
}

/* ===== 对外承诺的两个函数（真实项目里这两个原型要写进 .h）===== */
int a_compute(void);
void a_bump(void);

int a_compute(void)
{
    return g_shared + helper_seed() + scale_by_2(g_table[1]);   /* 100 + 1 + 40 */
}

void a_bump(void)
{
    g_shared += 7;             /* 修改的是全程序共享的那一个对象 */
}
