# C 语言基础语法（速查版）

> 这篇只讲**地基**：类型、运算符、控制流、函数、数组、输入输出。
> 刻意不讲透的部分都有归宿，别在这里找答案：
> 指针的算术与多级指针 → `01_Pointer_And_Memory/`；整型提升细节与 UB 边界 → `02_Type_System/`；`static/extern` 与链接 → `03_Scope_Storage_Linkage/`；宏 → `04_Preprocessor_Macro/`；字符串编码 → `05_String_Encoding/`；编译命令 → 根目录 `gcc_g++_cmake.md`。
> 全文代码块标 `待实测`：按 C11 写的，需要你在 `gcc 15.2` 上跑一遍确认，跑通后改成 `已验证`。

---

## 1. 一个 C 程序的最小骨架

```c
/* hello.c —— 注释只有这一种块注释；C 没有 // 之外的注释，但 C99 起支持 // */
#include <stdio.h>          /* 预处理指令：把 stdio.h 的内容原样插入此处 */

int main(void)              /* 整数返回类型 + 名为 main 的函数 = 程序入口 */
{
    printf("Hello, C\n");   /* \n 是转义字符；printf 不会自动换行 */
    return 0;               /* 返回给操作系统：0 = 成功 */
}
```

```bat
gcc -std=c11 -Wall -Wextra -g hello.c -o output\hello.exe && output\hello.exe
echo %ERRORLEVEL%           :: 查看上一条命令的退出码（Windows 用这个，不是 $?）
```

关于 `main` 必须知道的四件事：

1. 只有两种合法写法：`int main(void)` 和 `int main(int argc, char *argv[])`。`void main()` 不标准（MSVC 放过你，别学）。
2. `argc >= 1` 由标准保证，`argv[0]` 是程序名（可能为空字符串，但非 NULL），`argv[argc]` 必须是 `NULL`。
3. 从 `main` 里 `return 0;` 会触发正常终止（刷新所有流、调 `atexit` 注册的函数）。C99 起 `main` 末尾不写 return 也等价于 `return 0`，**但仅限 `main`**，其它函数漏 return 是 UB。
4. `int` 只有低 8 位对 shell 有意义（POSIX），Windows 的 `%ERRORLEVEL%` 是 32 位整数，别把返回值当数据用。

**翻译环境的三个层次**（`gcc_g++_cmake.md` 第 2 节的浓缩）：

```text
源代码字符集 → 执行字符集(编译进 exe 的字面量) → 控制台代码页(chcp)
     ↑                    ↑                          ↑
 -finput-charset    -fexec-charset              中文乱码的第三种原因
```

---

## 2. 类型与变量

### 2.1 整型：标准只保证"最小宽度"，实际宽度看平台

| 类型 | C 标准下限 | **x86-64 Windows(LLP64)** | x86-64 Linux(LP64) |
|---|---|---|---|
| `char` | 恰好 1 字节 | 1 | 1 |
| `short` | ≥16 位 | 2 | 2 |
| `int` | ≥16 位 | **4** | 4 |
| `long` | ≥32 位 | **4** | **8** ← 差异就在这 |
| `long long` | ≥64 位 | 8 | 8 |
| `size_t` | 无符号，够大 | **8** | 8 |
| `void*` / 指针 | — | 8 | 8 |
| `wchar_t` | 实现定义 | **2 (UTF-16)** | 4 (UTF-32) |

**`long` 在 Windows 和 Linux 上宽度不同**，这是"C 代码在 Windows 跑得好好的，拿到 WSL 就错乱"的头号元凶（尤其是 `printf("%ld", ...)` 打印 `size_t`）。解法是**永远用准确的标准类型 + 对应格式符**：

```c
#include <stdint.h>         /* int32_t / int64_t / uintptr_t：宽度写死，跨平台不猜 */
#include <stddef.h>         /* size_t / ptrdiff_t：由编译器定义，sizeof 的返回类型 */

size_t    n = sizeof(int);
ptrdiff_t d = &a[9] - &a[0];
printf("%zu %td\n", n, d);          /* ✅ 正确格式符 */
printf("%ld %d\n", n, d);           /* ❌ 类型和格式符不匹配：UB，且 Windows/Linux 后果不同 */
```

`stdint.h` 的固定宽度类型在 Windows/MinGW 上完全可用；如果只关心"至少这么大"，用 `int_fast32_t`/`int_least16_t`。它们由 `<limits.h>` 的宏（`INT_MAX`、`UINT32_MAX`、`SIZE_MAX`）配套。

### 2.2 signed / unsigned：`unsigned` 不是"用来表示正数"

`unsigned` 的语义是**模 2ⁿ 运算**。两个必须刻进脑子的后果：

```c
unsigned int u = 0;
u - 1;                    /* = 4294967295，不是 -1，不会报错 */

unsigned int i = 0;
for (; i <= 3u - 4u; ++i) { }      /* 条件恒为巨大数 → 死循环 */

int a = -7, b = 2;
a / b;                    /* -3：C99 起整除向零截断 */
a % b;                    /* -1：符号跟随被除数。(a/b)*b + a == b 恒等式成立 */
```

规则：只要有 `unsigned` 参与，另一个操作数就被**转成 unsigned**（`int + unsigned int` 结果是 `unsigned int`，即使 `int` 是负的）。这是 `for (i = 0; i < v.size() - 1; i++)` 之类的经典爆炸源（在 C++ 里同样成立）。

`char` 是否有符号**实现定义**（x86 GCC 默认 `signed char`）。要位模式就明写 `unsigned char`，要数值就明写 `signed char`。

### 2.3 字面量的类型：由"写法和大小"决定，不由你期望决定

```c
100            /* int */
100u           /* unsigned int */
100L           /* long（Windows 上就是 int，别以为它一定是 64 位）*/
100LL          /* long long —— 需要 64 位常量时必须写 LL */
0x7fffffff     /* int 上限；0x80000000 在 C 里是 unsigned int！*/
1.5            /* double，不是 float */
1.5f           /* float */
'A'            /* int！（C 里字符字面量类型是 int，C++ 里才是 char —— 高频困惑）*/
"hi"           /* char[3]{'h','i','\0'}，sizeof("hi")==3 */
"ab" "cd"      /* 相邻字面量拼接 → "abcd"，长度 5 */
```

后果示例：`printf("%d", 0x80000000);` 传的是 `unsigned`，格式符要 `%u`；`int x = 1 << 31;` 是有符号溢出 UB，应写 `1u << 31` 或用 `INT32_C(1) << 31`。

### 2.4 浮点：`0.1 + 0.2 != 0.3`，这不是 C 的 bug

```c
#include <float.h>        /* FLT_DIG=6, DBL_DIG=10, DBL_EPSILON≈2.2e-16 */

if (0.1 + 0.2 == 0.3) { }              /* 永远不成立：二进制无法精确表示 0.1 */

double got = 0.1 + 0.2;
if (fabs(got - 0.3) < 1e-12) { }       /* ✅ 用绝对误差 */
if (fabs(got - 0.3) <= 1e-9 * fmax(fabs(got), fabs(0.3))) { }  /* ✅✅ 相对误差，更稳 */
```

三条纪律：**别用 `==` 比浮点**；累加大量浮点数用 `double` 而不是 `float`（`float` 只有 6 位有效十进制数字，加一万个数误差足以显现）；`long double` 在 MinGW-w64 x86-64 上可能与 `double` 同宽（`待实测`：`printf("%zu\n", sizeof(long double))`）。

`nan`/`inf`：`<math.h>` 的 `isnan()`、`isinf()`、`isfinite()`，以及 `INFINITY`、`NAN` 宏。

### 2.5 声明、初始化与作用域

```c
int a;                  /* 块作用域里不初始化 = 值不确定，读它是 UB */
int b = 0;              /* 值初始化 */
static int c = 0;       /* 静态存储期：程序启动即为 0，无需显式初始化 */
const double PI = 3.14159265358979;   /* 只读约定，不是编译期常量（不能当数组长度）*/
enum Color { RED = 0, GREEN, BLUE };  /* 真正的常量表达式，可作数组维 */

int  n1, n2;            /* 一行声明多个 —— 可以，但别这么干 */
char *p1, p2;           /* ⚠️ p2 是 char，不是指针！这就是"别一行多声明"的原因 */
```

`const int a[10]` 里 `const` 修饰元素；`int *const q` 指针本身不可变；`const int *p` / `int const *p` 指向的内容不可变（**从右往左读**是唯一的可靠方法，见 `01_Pointer_And_Memory/`）。C 里 `const` 是**编译期约束**，不是运行期保证：`const int x = 1; *((int*)&x) = 2;` 能编译过（但这是 UB，别在真实代码里干）。真正要"值不变"用 `enum` 或 `#define`。

---

## 3. 运算符

### 3.1 只需要记住的相对顺序（从高到低）

```text
() [] -> .                        后缀，含 i++ / i--
!  ~  ++ --  + -  *  &  sizeof   _Alignof   (T)   一元
*  /  %                           乘除模（同级，左结合）
+  -                              加减
<<  >>                            移位
<  <=  >  >=                      关系
==  !=                            相等
&                                 按位与
^                                 按位异或
|                                 按位或
&&                                逻辑与
||                                逻辑或
?:                                三目（右结合）
=  +=  -=  ...                    赋值（右结合）
,                                 逗号（最低）
```

背不住就**加括号**，没人给你发整洁代码奖。真正要背的是这三条分界线：`&` 低于 `==`、`&&` 低于 `|`、赋值低于一切比较。于是：

```c
if (flags & MASK == 1)      /* ❌ 解析成 flags & (MASK==1) —— 静默错值 */
if ((flags & MASK) == 1)    /* ✅ */

while (p != NULL)           /* 比较高于赋值，所以这里是对的 */
if (p = next)               /* 合法但危险：赋值给条件。多数的 lint 会拦 */
if ((p = next) != NULL)     /* ✅ 想这么写得把括号补齐，让意图可见 */
```

### 3.2 短路：不只是快，是**安全机制**

```c
if (p != NULL && p->name != NULL && strcmp(p->name, key) == 0)   /* ✅ 左侧假则右侧不求值 */
if (p != NULL & p->name != NULL)                                 /* ❌ & 不短路，p 为空时已崩溃 */
```

`&&`、`||`、`?:`、`,` 四个运算符是**序列点**：左操作数完整求值后，右操作数才开始。其它地方**求值顺序未规定**，于是：

```c
int i = 0;
i = i++;                     /* UB：同一标量对象在序列点间被修改两次 */
int a[] = { i++, i++ };      /* 未指定顺序，结果可能是 {0,1} 或 {1,0} */
printf("%d %d\n", i, i++);   /* UB */
x = f(i) + g(i++);           /* UB */
```

GCC 对这些给 `-Wsequence-point`（在 `-Wall` 里）。看到警告**别去猜结果**，拆成两条语句。

### 3.3 移位与位运算的隐藏规则

```c
1 << 31        /* 有符号溢出 → UB。写 1u << 31 */
-1 >> 1        /* 对有符号数右移是实现定义（GCC/x86 是算术右移，保持符号）*/
1 << 64        /* 移位量 >= 类型宽度 → UB（x86 上实际只取低 6 位，静默得 1<<0）*/
```

常用位技巧（记模式，不记代码）：`x & (x-1)` 去掉最低位的 1；`(x & (x-1)) == 0 && x != 0` 判 2 的幂；`x ^ y` 交换（慎用）；`~0u` 全 1；`x & -x` 取最低位的 1。

### 3.4 `sizeof` 与结构体填充：直接影响你的内存布局

```c
sizeof x        /* 括号不是必须的（除 sizeof(type)）*/
sizeof(a)       /* 数组：整个数组字节数 */
sizeof(a+0)     /* 指针：8。数组名参与运算立刻退化 */
sizeof(n)/sizeof(n[0])   /* 元素个数惯用法，仅对真数组有效，对指针参数失效！ */
```

```c
struct Bad  { char a; int b; char c; };     /* 2 + 4 + 2 → 12 字节，不是 8 */
struct Good { int b; char a; char c; };     /* 4 + 2 → 8 字节 */
```

`int` 需要 4 字节对齐，编译器在 `a` 后塞 3 字节 padding。把成员**按宽度降序排**能省不少内存；`#pragma pack(1)` / `__attribute__((packed))` 能强行紧凑，代价是访问变慢且**取 packed 成员地址可能出问题**。网络协议/文件格式解析要用它，同时用 `<stdint.h>` + 手工字节序转换（细节见 `01_Pointer_And_Memory/`、`05_String_Encoding/`）。

---

## 4. 控制流

### 4.1 `if` / `switch`

```c
if (x > 0) { }
else if (x == 0) { }        /* C 没有 elif，也别写 elseif */
else { }

switch (ch) {               /* ⚠️ 只接受整型（含 char/enum）；不能 switch 字符串/浮点 */
case 'a':
case 'A':                   /* 故意穿透：合并 case 是常见正用 */
    handle_a();
    break;                  /* 忘 break = 继续执行下一个 case，新手第一大坑 */
default:
    handle_other();
    break;                  /* default 放最后时 break 可省，但留着更保险 */
}
```

`case` 值必须是**整型常量表达式**（`enum` 成员可以，`const int` 不行——`const` 在 C 里不是编译期常量）。
开 `-Wimplicit-fallthrough`（在 `-Wextra` 里就有），并在有意穿透处写 `/* fall through */`，GCC 认这个注释。
比较字符串永远用 `strcmp`，`switch` 想匹配字符串就先自己做哈希（见 `10_DS_Hash/`）。

### 4.2 循环

```c
while (cond) { }            /* 可能一次都不执行 */
do { } while (cond);        /* 至少执行一次：读入并校验参数的标准形状 */

int c;
do { c = getchar(); } while (c != EOF && !isdigit(c));   /* 跳过非数字 */

for (int i = 0; i < n; ++i) { }      /* C99 起可在 for 里声明，i 的作用域仅限循环 */
for (;;) { }                          /* 无限循环惯用写法（比 while(1) 不产生警告）*/
```

`break` 只跳**最内层**一层。跳出多层嵌套：

```c
for (i = 0; i < n; ++i)
    for (j = 0; j < m; ++j)
        if (found(i, j))
            goto done;                /* goto 的正当用途之一：跳出多重循环 */
done:
    ;
```

`goto` 的正当用途之二——**单一清理出口**（C 没有析构函数和异常，这是标准替代方案）：

```c
int parse(FILE *f, char **out, size_t *len)
{
    char  *buf = NULL;
    FILE  *tmp = NULL;
    int    rc  = -1;

    buf = malloc(CAP);
    if (!buf) goto out;
    tmp = fopen("log", "r");
    if (!tmp) goto out;
    /* ... 真正的工作 ... */
    rc = 0;
out:
    if (tmp) fclose(tmp);             /* 逆序释放，只写一次 */
    free(buf);
    return rc;
}
```

这个"资源获取即失败则 `goto out`、出口集中释放"的模式在 C 项目里到处都是（OpenSSL、Redis、内核），第一次读会觉得绕，读三遍就变成肌肉记忆。

### 4.3 没有的东西（别按其它语言的习惯找）

| 你想要的 | C 里怎么办 |
|---|---|
| `for x in list` | 索引循环，或链表手写 `for (p = h; p; p = p->next)` |
| `bool` 容器/`None` | `int`/`_Bool` 当标志；指针 `NULL` 当"无" |
| `try/except` | 返回码 + `errno`（见 `12_Generic_Container/`）；`setjmp/longjmp` 慎用 |
| 函数重载 | 没有。用不同名字（`atoi/atof/atol`）或 `_Generic` |
| 默认参数 | 没有。多写一个包装函数 |
| 三元以外的模式匹配 | `switch` + 函数表 |

---

## 5. 函数

### 5.1 声明与定义：分离是 C 的骨架

```c
/* vector.h —— 只放声明（extern 语义），可以被无数个 .c 包含 */
typedef struct Vec Vec;                    /* 不透明指针：外部看不到内部字段 */
Vec  *vec_create(size_t cap);
void  vec_free(Vec *v);                    /* 所有权：调完这个函数，指针归我管 */
int   vec_push(Vec *v, int value);         /* 返回 0 成功 / -1 失败 —— 必须查 */

/* vector.c —— 定义在这里，整个项目只能有一份 */
```

**没写原型就调用**是 C 的历史包袱，后果具体可见：

```c
float f(void) { return 3.14f; }
int main(void) {
    printf("%f\n", f());      /* ❌ 未声明 → 编译器假设返回 double，还做默认参数提升 */
}
```

老式规则叫**默认参数提升**：`char`/`short` → `int`，`float` → `double`。类型猜错时值会**静默变垃圾**（尤其在 x86-64 的向量寄存器和整数寄存器之间串线）。所以 `-Wall` 下的 `implicit-function-declaration` 警告必须当错误处理。

`int f(void)` 与 `int f()` 不是一回事：

```c
int f(void)   /* 明确"不接受任何参数" —— 永远这么写 */
int f()       /* 老式：不检查参数个数和类型（C23 已把 () 改成等价 (void)）*/
```

### 5.2 按值传递：C 只有一种传参方式

```c
void bad(int n)    { n = 42; }              /* 改了副本，外面看不见 */
void good(int *pn) { *pn = 42; }            /* 传地址，改的是本体 */
int  main(void) { int x = 0; good(&x); }    /* 取地址交给被调方 = "输出参数" */
```

想"返回多个值"就传指针；想返回大对象就返回堆指针（并**在注释里写清谁负责 free**）或让调用方传缓冲区（更安全，Redis/内核风格）：

```c
size_t fmt_time(char *buf, size_t buflen, const struct tm *t);  /* 写入调用方的缓冲区 */
```

**绝对不要返回局部变量的地址**——函数返回后那块栈就作废了，指针悬空。GCC 会警告 `-Wreturn-local-addr`，但同类问题（返回指向已 `free` 内存的指针）警告不出来。

### 5.3 函数指针：C 唯一的"多态"，也是回调的全部实现

```c
typedef int (*CmpFn)(const void *, const void *);     /* 类型名让代码可读，别裸写 */

CmpFn cmp = strcmp_safe;                              /* 函数名自动转成指针 */
if (cmp(a, b) < 0) { }

/* 分发表：代替超长的 if-else 链 */
static const struct { const char *name; CmpFn fn; } OPS[] = {
    { "int",  cmp_int  },
    { "str",  cmp_str  },
    { "bytes", cmp_bytes },
};

/* 标准库就靠它做泛型：qsort */
qsort(arr, n, sizeof(arr[0]), cmp_int);
```

`int (*a[5])(int)` 是"含 5 个函数指针的数组"，`int (*f)(int)` 是"一个函数指针"——从里往外读，规则见 `01_Pointer_And_Memory/`。

### 5.4 可变参数：能用就少用

```c
#include <stdarg.h>
int sum(int count, ...)                 /* 必须有一个"数量"或哨兵告诉我在哪停 */
{
    va_list ap;
    int s = 0;
    va_start(ap, count);                /* 第二个参数是最后一个具名参数 */
    for (int i = 0; i < count; ++i) s += va_arg(ap, int);
    va_end(ap);                         /* 忘了 va_end 在某些实现会泄漏栈 */
    return s;
}
```

坑：`char`/`short`/`float` 传进来已被提升，所以 `va_arg(ap, int)` 取 `char` 是对的、`va_arg(ap, char)` 是错的。C 不会检查类型，写错就是 UB。`printf` 的格式串才是唯一的"类型来源"。
GCC 能让编译器检查你自己的格式化函数：

```c
__attribute__((format(printf, 1, 2))) void mylog(const char *fmt, ...);
/*                                                     ↑格式串是第1参 ↑变参从第2个起 */
```

加上它，`mylog("%d", someString)` 会立刻报警告。**强烈建议给所有 printf 型封装加这个属性**。

`_Generic`（C11）是更安全的选择，见 `04_Preprocessor_Macro/`。

### 5.5 其它常用属性与 `inline`

```c
static int helper(void);                     /* 内部链接：别的 .c 看不见，编译器能更激进优化 */
_Noreturn void die(const char *fmt, ...);    /* C11：告知永不返回，-Wreturn-type 会更聪明 */
inline int fast(int x);                      /* C99 inline 语义有坑：不等于"强制内联"，见 03 目录 */
void f(int *restrict a, const int *restrict b);  /* 承诺不重叠 → 允许向量化 */
```

---

## 6. 数组

### 6.1 三条基本事实

```c
int a[5] = {0};                 /* 全部 0；{1,2} 其余补 0 */
a[-1] = 1;                      /* 不报错！C 不做边界检查，越界是 UB */
int n = (int)(sizeof a / sizeof a[0]);   /* 元素个数 —— 只在数组还"是数组"的地方有效 */
```

下标就是指针运算的语法糖：`a[i]` 恒等于 `*(a + i)`，甚至 `i[a]` 合法（同一个表达式）。**数组不会自动扩张，也不会告诉你长度**。传进函数就退化成指针，`sizeof` 立刻变 8：

```c
void process(int a[], size_t n);      /* 声明里 int a[] 与 int *a 完全等价 */
void process(int n, int a[static 10]);/* C99：static 告诉编译器"至少 10 个"，越界检查友好些 */

process(a, n);                        /* 永远显式传长度。这是 C 接口的第一原则 */
```

### 6.2 多维数组是"一维数组的数组"

```c
int m[2][3] = { {1,2,3}, {4,5,6} };
/* 内存里连续：1 2 3 4 5 6，行优先 */

&m[0][0]              /* int*        首元素 */
m + 1                 /* int (*)[3]  跳到下一行（+12 字节，不是 +4！）*/
int (*p)[3] = m;      /* 指向"含3个int的数组"的指针 —— p[i][j] 正常工作 */
int *q = &m[0][0];    /* 展平访问：q[i * 3 + j] */
```

`m[i][j] == *(&m[0][0] + i * 3 + j)`——**列数必须是编译期已知的常量**，这是为什么"动态二维数组"在 C 里要么用一维 + 手算下标（推荐，缓存友好），要么指针数组（每行单独 malloc，不连续）。展开见 `01_Pointer_And_Memory/`。

### 6.3 `char` 数组与字符串

```c
char s1[6] = "hello";       /* 含终止 \0，sizeof==6 */
char s2[]  = "hello";       /* 数组长度由字面量决定 = 6 */
char s3[5] = "hello";       /* ⚠️ 恰好被截掉 \0 —— printf("%s") 会读越界 */
char s4[]  = { 'h','i',0 }; /* 手写终止符的等价形式 */

const char *lit = "hello";  /* 指向只读区；写它是 UB：strcpy(lit,"x") 会崩 */
```

`strncpy(dst, src, n)` **不保证补 `\0`**（截断时不补），这是最常见的安全错觉来源。正确用法：

```c
strncpy(dst, src, sizeof dst - 1);
dst[sizeof dst - 1] = '\0';                 /* 自己补 —— 或者直接用 snprintf */
snprintf(dst, sizeof dst, "%s", src);       /* ✅ 永远截断安全且必然补 \0 */
```

`gets()` 因无法限制长度已从 C11 删除；读一行永远用 `fgets(buf, sizeof buf, f)`，注意它**会把 `\n` 也存进缓冲区**：

```c
buf[strcspn(buf, "\r\n")] = '\0';           /* 惯用法：原地去掉行尾换行 */
```

`<string.h>` 全家桶、性能与陷阱见 `05_String_Encoding/`。

### 6.4 VLA 与复合字面量

```c
int n; scanf("%d", &n);
int vla[n];             /* C99 变长数组：在栈上，长度运行时才知道 */
                        /* C11 起变成可选特性(Annex K)，C23 已移除，MSVC 从来不支持 */
                        /* 别拿它放大数组（栈溢出），也别依赖它可移植 */

int *p = (int[]){ 1, 2, 3 };                /* 复合字面量：创建一个匿名数组，生命周期=所在块 */
struct Point o = (struct Point){ .x = 1, .y = 2 };  /* 指定初始化器 */
int sparse[10] = { [0]=1, [5]=9, [9]=3 };   /* 乱序按索引初始化 */
```

指定初始化器（`.field =`）是 C 里最好用的特性之一：**结构体加字段不破坏旧代码**，且未列出的字段自动置 0。写库接口时优先用它。

---

## 7. 输入输出

### 7.1 三个流与缓冲：顺序错乱的原因

```text
stdin   行缓冲          --stdin
stdout  终端=行缓冲 / 重定向到文件或管道=全缓冲
stderr  无缓冲（立刻可见）
```

```bat
output\app.exe > log.txt 2> err.txt
```

重定向后 `stdout` 变成**全缓冲**（攒满约 4KB 才吐），于是你看到"错误信息出现在正常信息前面"——不是程序错乱，是缓冲时序。崩溃时缓冲区里最后一段 `printf` 直接丢失。控制它：

```c
fflush(stdout);              /* 手动吐出去 */
fflush(NULL);                /* 刷新所有输出流 */
setvbuf(stdout, NULL, _IONBF, 0);   /* 关缓冲：调试期让输出顺序符合直觉 */
```

### 7.2 `printf` 格式符：按类型选，不要猜

```text
%d int          %u unsigned int     %ld long      %lld long long
%zu size_t      %td ptrdiff_t       %ju uintmax_t
%x %X %o        十六进制/八进制     %p void*     （严格说应转成 (void*)）
%f double       %e 科学计数   %g 自动选   %c int(打印一个字符)   %s char*
%%  百分号本身
```

修饰部分：`%[flag][width][.prec][len]conv`

```c
printf("%-10s|\n", "ab");        /* 左对齐补空格 → "ab        |" */
printf("%08X\n", 0x1u);          /* 0 填充 */
printf("%+d % d\n", 5, 5);       /* 强制符号 / 空格占位（对齐正负数好看）*/
printf("%.3f | %10.2e\n", 3.14159, 1.2e5);
printf("%.*s\n", 5, "abcdefgh"); /* ✅ 动态精度：运行时决定截断长度，写库时极有用 */
```

Windows/MinGW 专属：`%zu`/`%lld` 依赖运行时库的 ANSI 兼容层。若发现它们打出 `0` 或错值，显式开启：`gcc -D__USE_MINGW_ANSI_STDIO=1 ...`。**待实测**——你的 GCC 15 大概率默认已开，跑一遍 `printf("%zu %lld\n", (size_t)1, 1LL);` 即可确认。

### 7.3 `scanf`：返回值必须查，这是它和 `printf` 最大的区别

```c
int n;
if (scanf("%d", &n) != 1) {              /* 返回"成功赋值的项目数"，不是 0/1 */
    fprintf(stderr, "需要输入一个整数\n");
    return 1;
}
```

四个必踩的坑：

```c
scanf("%d", n);              /* ❌ 忘了 & → 把 n 的值当地址写 → 崩溃 */
scanf("%c", &ch);            /* ❌ 前面 scanf("%d") 留下的 '\n' 会被 %c 直接吃掉 */
scanf(" %c", &ch);           /* ✅ 格式串前加空格 = 跳过所有空白 */
while (getchar() != '\n') ;  /* ✅ 或者干脆清掉行尾残留 */
```

`scanf("%s", buf)` **不限制长度 → 缓冲区溢出**，和 `gets` 同罪。必须写宽度，而且宽度是"最大字符数-1"：

```c
char buf[32];
scanf("%31s", buf);                  /* ✅ 留一位给 \0 */
fgets(buf, sizeof buf, stdin);       /* ✅✅ 更推荐：整行读入再自己解析 */
```

**工程上的正确做法**：用 `fgets` 读一整行，再用 `sscanf`/`strtol` 解析。这样解析失败时你**还留着原始文本**可以报错，而 `scanf` 失败后输入流会卡在那个坏字符上无限循环（`while (scanf("%d",&n)!=1) scanf("%*s");` 这种补救很脆）。

`getchar()` 的返回类型必须是 `int`，不是 `char`：

```c
int c;                                  /* ✅ EOF 通常是 -1，char 装不下或会混淆 0xFF */
while ((c = getchar()) != EOF) putchar(c);
```

数字解析用 `strtol` 而不是 `atoi`——`atoi` **无法报告错误**（坏输入返回 0，和合法的 0 分不开，溢出不检测）：

```c
errno = 0;
char *end;
long v = strtol(text, &end, 10);        /* 0x 前缀自动按 16 进制 */
if (end == text || *end != '\0' || errno == ERANGE) { /* 解析失败 */ }
```

`FILE*`、`fread/fwrite`、文本与二进制模式、`fseek` 与 `feof` 的滞后一位问题 → `06_File_IO/`。

---

## 8. 指针与动态内存：本节只给"最小可用"

```c
int   x = 10;
int  *p = &x;          /* p 存的是 x 的地址；sizeof p == 8 */
*p = 20;               /* 解引用改写 x */

int  *q = malloc(5 * sizeof *q);   /* ✅ sizeof *q 优于 sizeof(int)：改类型不会漏改 */
if (!q) return -1;                 /* 检查 NULL —— 每次都要 */
q[0] = 1;                          /* malloc 返回的指针可直接下标 */
free(q);
q = NULL;                          /* ✅ 防止二次 free / 悬空读 */
```

三条纪律，违反任何一条都会在最远的地方爆炸：

1. **每个 `malloc/calloc/realloc` 都对应**且只对应一次 `free`。
2. **`free` 之后立刻置 `NULL`**，不再使用那个指针（`free(NULL)` 安全，是空操作）。
3. **谁创建谁释放，或者在接口注释里明确写"所有权转移给调用方"**。90% 的 C 内存 bug 源于所有权没说清。

`realloc` 的正确用法（失败时不能丢原指针）：

```c
int *tmp = realloc(q, newcount * sizeof *q);
if (!tmp) { /* 处理失败，q 仍然有效 */ }
else      { q = tmp; }
```

四种经典内存错误，以及它们**延迟发作**的特点：

| 错误 | 表现 |
|---|---|
| 越界读写 | 可能当场崩，也可能几个月后在无关处崩 |
| double free | 崩溃在 `free` 内部（`glibc`/msvcrt 堆元数据被破坏） |
| use-after-free | 读到奇怪的旧值 / `0xfeeefeee`(调试堆填充值) |
| 泄漏 | 不崩，内存持续增长直到系统杀进程 |

`待实测`：`-fsanitize=address` 在你这套 MinGW 上是否可用（命令见 `gcc_g++_cmake.md` 12.4）。不可用就用 gdb 的 `watch` 断点 + `14_Memory_Management/` 的手写调试分配器。指针的算术、多级指针、函数指针、结构体 padding 全部在 `01_Pointer_And_Memory/`。

---

## 9. 高频警告→含义对照（每条都别忽略）

```text
unused variable 'x' / unused parameter 'p'     忘了写，还是名字拼错？参数用 (void)p; 显式弃用
variable 'x' is used uninitialized             真 bug 概率极高，读它 = UB
statement with no effect                       写了 x+1; 而不是 x += 1;
implicit declaration of function              忘了 #include 或拼错函数名
initialization of 'int' from 'char *'         类型串线
subscripted value is neither array nor pointer 把 int 当下标用了：x[3] 里 x 不是数组
assignment to expression with aggregate type   数组不能整体赋值，用 strcpy/memcpy
control reaches end of non-void function       某条分支没有 return —— UB
missing braces around initializer              嵌套结构体/二维数组少写一层 {}
format '%d' expects argument of type 'int', but argument 2 has type 'long int'  格式符与实参不匹配
comparison between signed and unsigned         隐式转 unsigned，负数变巨大
pointer of type 'void *' used in arithmetic    GCC 扩展，标准不允许
```

看到警告的正确反应不是"消掉它"，是**问自己：如果编译器是对的，我的代码哪里错了我却没看出来**。

---

## 10. 自测与练习

**能秒答说明基础过关**：

1. `sizeof("C语言")` 在源文件 UTF-8、`-fexec-charset=UTF-8` 下等于几？为什么？
2. `char c = 200; printf("%d\n", c);` 输出什么？为什么和 `signed`/`unsigned` 有关？
3. `int a[3][4]; a + 1` 和 `&a[0][0] + 1` 分别前进多少字节？
4. `for (unsigned i = 5; i >= 0; --i)` 会结束吗？
5. `scanf("%2s", buf)` 里 `2` 的含义和缓冲区大小的关系？
6. 为什么 `fopen("a.txt","w")` 之后 `fseek` 到 0 再读会读到旧内容或失败？（提示：`w` 与 `r+`/`w+` 区别）
7. 函数里 `return local_array;` 为什么危险？给出两种正确改法。

**动手练习（做完再进 `01_Pointer_And_Memory/`）**：

- **P1 输入健壮性**：写 `read_int(prompt)`，循环要求用户输入整数，非法输入重新提示；用 `fgets`+`strtol` 实现，能处理 `12abc`、`-0`、`  42  `、超过 `INT_MAX` 的数。
  *验收*：粘贴 `abc`、`99999999999999999999`、空行都不能崩、不能死循环。
- **P2 手工统计**：`fgets` 逐行读文件，统计行数、单词数、字节数（做简化版 `wc -lwc`），处理无尾换行的最后一行。
  *验收*：与 PowerShell `Measure-Object` 或 MSYS2 `wc` 结果一致。
- **P3 类型体检**：打印你这个平台上的 `sizeof(char/short/int/long/long long/size_t/void*/float/double/long double/wchar_t)` 与 `<limits.h>` 的 `INT_MAX`、`UINT32_MAX`、`SIZE_MAX`。
  *验收*：和你背的 LLP64 表格逐项对上，不一致就问"是编译器还是我的理解错了"。
- **P4 警告清零**：故意写下第 9 节列的 5 种错误，编译并**把每条警告的原文抄进笔记**，然后修好它们。



