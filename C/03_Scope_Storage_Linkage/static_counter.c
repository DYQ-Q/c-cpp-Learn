/* static_counter.c —— 块作用域 static / 文件作用域 static / static inline 的实测
 *
 * 编译：gcc -std=c11 -Wall -Wextra -Wpedantic -g static_counter.c -o ..\output\static_counter.exe
 * 对照：gcc -std=c11 -O2 static_counter.c -o ..\output\static_counter_O2.exe
 *       （加 -Wall 时留意：未被使用的 static 函数会被警告，这是"内部链接可被安全删除"的证据）
 * 【待实测】
 */
#include <stdio.h>

/* ---------- 文件作用域：内部链接的模块私有状态 ---------- */
static int s_state;                 /* 零初始化，进 .bss；别的 TU 看不见这个名字 */

int  module_value(void);            /* 外部链接的访问器（真实项目里这句应放头文件） */
void module_reset(void);
void module_add(int delta);

int  module_value(void)  { return s_state; }
void module_reset(void)  { s_state = 0; }
void module_add(int d)   { s_state += d; }

/* ---------- 块作用域 static：活一辈子，但只在函数内可见 ---------- */
static int next_call_number(void)
{
    static int counter;              /* ⚠️ 只初始化一次（零初始化），跨调用保持 */
    return ++counter;
}

/* 每次进入都有新对象：自动存储期，无链接 */
static int fresh_each_time(void)
{
    int counter = 0;                 /* 每次调用都重置为 0 */
    return ++counter;
}

/* 静态存储期对象的初始化必须是常量表达式 —— 下面这行如果取消注释会编译报错：
 *   static int bad = rand();        // ❌ Error: initializer element is not constant
 * 想要"运行时算初值"，就写成懒初始化（注意：懒初始化在多线程下不是原子的！见 17_Concurrency）*/
static int lazy_value(void)
{
    static int initialized = 0;
    static int value = 0;
    if (!initialized) {
        value = 6 * 7;
        initialized = 1;
    }
    return value;
}

/* ---------- static inline：头文件工具函数的唯一安全写法 ---------- */
static inline int clamp_int(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/* ---------- 线程局部：C11 存储期家族的一员（本文件单线程，只证明它可用） ---------- */
_Thread_local static int tls_counter;

int main(void)
{
    puts("== 1. 块作用域 static 只初始化一次 ==");
    int a = next_call_number();
    int b = next_call_number();
    int c = next_call_number();
    int f1 = fresh_each_time();
    int f2 = fresh_each_time();
    int f3 = fresh_each_time();
    printf("  连续三次 next_call_number() = %d %d %d（期望 1 2 3）\n", a, b, c);
    printf("  连续三次 fresh_each_time()  = %d %d %d（期望 1 1 1：自动存储期每次都是新对象）\n",
           f1, f2, f3);

    puts("== 2. 未指定行为：写成一行 printf 会看到顺序漂移 ==");
    printf("  （下面这行故意把三个有副作用的调用放进一次 printf —— 实参求值顺序未指定）\n");
    printf("  可能输出 3 2 1 / 1 2 3 / 1 3 2 ...：%d %d %d\n",
           next_call_number(), next_call_number(), next_call_number());
    puts("  结论：需要确定顺序就分三条语句。这就是 C 的 'unspecified order'，不是 UB。");

    puts("== 3. 文件作用域 static + 访问器 ==");
    module_add(5);
    module_add(37);
    printf("  module_value() = %d（期望 42）\n", module_value());
    module_reset();
    printf("  reset 后 = %d（期望 0）\n", module_value());

    puts("== 4. 懒初始化 ==");
    printf("  lazy_value() 两次 = %d %d（期望 42 42：第二次不再计算）\n",
           lazy_value(), lazy_value());

    puts("== 5. static inline 工具函数 ==");
    printf("  clamp_int(-5,0,10)=%d clamp_int(5,0,10)=%d clamp_int(99,0,10)=%d\n",
           clamp_int(-5, 0, 10), clamp_int(5, 0, 10), clamp_int(99, 0, 10));

    puts("== 6. _Thread_local（单线程下等价于普通 static）==");
    tls_counter += 10;
    printf("  tls_counter = %d（期望 10；多线程时每线程各一份，见 17_Concurrency）\n", tls_counter);

    puts("== 7. 动手验证链接性（在 cmd 里跑）==");
    puts("  gcc -std=c11 -c static_counter.c -o ..\\output\\sc.o");
    puts("  nm ..\\output\\sc.o");
    puts("  期望看到：module_value/module_add/module_reset 是大写 T（外部链接）");
    puts("            s_state 是小写 b 或 d（内部链接）；next_call_number 是小写 t");
    puts("  换 32 位编译（-m32）时符号还会多一个前导下划线：_module_value 【待实测】");
    return 0;
}
