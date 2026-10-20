# 1. C语言函数简介

函数是 C 语言的基本组成单元：

```text
封装可复用的代码块
接收输入（参数），返回输出（返回值）
```

函数的作用：

```text
• 代码复用，避免重复
• 模块化，提高可维护性
• 抽象细节，隐藏实现
• 分工协作，多人开发
```

与变量的关系：

```text
变量存储数据，函数处理数据
```

例如：

```c
int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(3, 5);   // 调用函数
    printf("%d\n", result);   // 8
    return 0;
}
```

---

# 2. 函数的基本语法

## 2.1 函数定义

```c
返回类型 函数名(参数列表) {
    // 函数体
    return 返回值;   // 若返回类型为 void，可省略
}
```

示例：

```c
int add(int a, int b) {
    return a + b;
}

double average(double x, double y) {
    return (x + y) / 2.0;
}
```

---

## 2.2 函数声明（原型）

```c
// 声明：告诉编译器函数的存在
int add(int a, int b);
void print_hello(void);

// 定义可以在后面
int main() {
    int result = add(3, 5);   // 编译器已知 add 的签名
    return 0;
}

int add(int a, int b) {
    return a + b;
}
```

声明与定义分离的意义：

```text
• 支持多文件编译
• 头文件中放声明，源文件中放定义
• 避免"隐式声明"警告
```

---

## 2.3 函数调用

```c
int result = add(3, 5);      // 有返回值
print_hello();                // 无返回值
```

---

## 2.4 函数的组成要素

```text
• 返回类型：函数返回值的类型
• 函数名：标识符，遵循命名规则
• 参数列表：输入数据的类型和名称
• 函数体：执行的代码块
• 返回值：通过 return 返回
```

---

# 3. 返回类型

## 3.1 void 函数（无返回值）

```c
void print_hello(void) {
    printf("Hello\n");
}

void print_number(int n) {
    printf("%d\n", n);
}
```

调用：

```c
print_hello();
print_number(10);
```

**注意**：void 函数中可以使用 `return;` 提前退出：

```c
void check(int x) {
    if (x < 0) {
        printf("负数\n");
        return;      // 提前返回
    }
    printf("非负数\n");
}
```

---

## 3.2 基本类型返回值

```c
int square(int x) {
    return x * x;
}

double half(double x) {
    return x / 2.0;
}

char first_char(const char *s) {
    return s[0];
}
```

---

## 3.3 返回指针的函数（指针函数）

返回指针的函数，注意与"函数指针"区分。

```c
// 返回 int* 的函数
int* find_max(int *arr, int size) {
    int *max = &arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > *max) {
            max = &arr[i];
        }
    }
    return max;
}

int main() {
    int arr[] = {3, 7, 1, 9, 4};
    int *p = find_max(arr, 5);
    printf("%d\n", *p);   // 9
    return 0;
}
```

**危险**：不要返回局部变量的地址：

```c
int* bad() {
    int a = 10;
    return &a;      // 未定义行为：a 在函数返回后销毁
}
```

安全的返回方式：

```c
// 1. 返回静态变量（注意线程安全）
int* safe1() {
    static int a = 10;
    return &a;
}

// 2. 返回堆内存（调用者负责 free）
int* safe2() {
    int *p = (int*)malloc(sizeof(int));
    *p = 10;
    return p;
}

// 3. 返回传入的指针
int* identity(int *p) {
    return p;
}
```

---

## 3.4 返回结构体的函数

```c
typedef struct {
    int x;
    int y;
} Point;

Point make_point(int x, int y) {
    Point p = {x, y};
    return p;       // 返回结构体副本
}

int main() {
    Point p = make_point(3, 4);
    printf("%d %d\n", p.x, p.y);   // 3 4
    return 0;
}
```

**注意**：大结构体返回有拷贝开销，可用指针返回：

```c
void make_point(Point *p, int x, int y) {
    p->x = x;
    p->y = y;
}

Point p;
make_point(&p, 3, 4);
```

---

# 4. 参数类型

## 4.1 值传递（基本类型）

```c
void change(int x) {
    x = 100;      // 只修改副本
}

int main() {
    int a = 10;
    change(a);
    printf("%d\n", a);   // 10（不变）
    return 0;
}
```

**C 语言只有值传递**，基本类型传入的是值的副本。

---

## 4.2 指针参数（传址）

```c
void change(int *x) {
    *x = 100;     // 通过指针修改原值
}

int main() {
    int a = 10;
    change(&a);
    printf("%d\n", a);   // 100
    return 0;
}
```

交换函数：

```c
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int x = 5, y = 10;
swap(&x, &y);   // x=10, y=5
```

---

## 4.3 数组参数

数组作为参数时退化为指针：

```c
void print_array(int arr[], int size) {
// 等价于 void print_array(int *arr, int size)
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
}

int main() {
    int data[] = {1, 2, 3, 4, 5};
    print_array(data, 5);
    return 0;
}
```

**注意**：`sizeof(arr)` 在函数内得到的是指针大小（8），不是数组大小，必须额外传 `size`。

---

## 4.4 二维数组参数

```c
void print_matrix(int mat[][4], int rows) {
// 等价于 void print_matrix(int (*mat)[4], int rows)
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < 4; j++) {
            printf("%d ", mat[i][j]);
        }
        printf("\n");
    }
}

int main() {
    int m[2][4] = {{1,2,3,4}, {5,6,7,8}};
    print_matrix(m, 2);
    return 0;
}
```

**注意**：第二维必须指定（如 `[4]`），因为编译器需要知道行长度。

---

## 4.5 结构体参数

### 值传递

```c
void print_point(Point p) {    // 复制整个结构体
    printf("%d %d\n", p.x, p.y);
}
```

**缺点**：大结构体复制开销大。

### 指针传递（推荐）

```c
void move(Point *p, int dx, int dy) {
    p->x += dx;
    p->y += dy;
}

Point p = {3, 4};
move(&p, 1, 2);
```

### const 指针（只读）

```c
void print_point(const Point *p) {   // 不修改，避免复制
    printf("%d %d\n", p->x, p->y);
}
```

---

## 4.6 指针的指针参数

用于修改指针本身：

```c
void allocate(int **p, int size) {
    *p = (int*)malloc(size * sizeof(int));
}

int main() {
    int *arr = NULL;
    allocate(&arr, 10);
    // 使用 arr
    free(arr);
    return 0;
}
```

---

## 4.7 函数指针参数（回调）

```c
int add(int a, int b) { return a + b; }
int mul(int a, int b) { return a * b; }

// 接收函数指针作为参数
int calculate(int a, int b, int (*op)(int, int)) {
    return op(a, b);
}

int main() {
    printf("%d\n", calculate(3, 5, add));   // 8
    printf("%d\n", calculate(3, 5, mul));   // 15
    return 0;
}
```

---

## 4.8 void 指针参数（泛型）

```c
void swap_generic(void *a, void *b, size_t size) {
    char temp[size];    // C99 变长数组
    memcpy(temp, a, size);
    memcpy(a, b, size);
    memcpy(b, temp, size);
}

int x = 10, y = 20;
swap_generic(&x, &y, sizeof(int));
```

常用于通用函数（`memcpy`、`qsort`）：

```c
int cmp(const void *a, const void *b) {
    return *(int*)a - *(int*)b;
}

int arr[] = {3, 1, 4, 1, 5};
qsort(arr, 5, sizeof(int), cmp);
```

---

## 4.9 参数类型总结

| 参数类型 | 语法 | 特点 |
| -------- | ---- | ---- |
| 基本类型 | `int x` | 值传递，副本 |
| 指针 | `int *p` | 传址，可修改原值 |
| 数组 | `int arr[]` | 退化为指针 |
| 二维数组 | `int arr[][4]` | 第二维必须指定 |
| 结构体 | `struct S s` | 复制整个结构体 |
| 结构体指针 | `struct S *p` | 推荐，避免复制 |
| const 指针 | `const T *p` | 只读，不修改 |
| 二级指针 | `int **p` | 修改指针本身 |
| 函数指针 | `int (*f)(int)` | 回调 |
| void 指针 | `void *p` | 泛型 |

---

# 5. 参数传递的高级特性

## 5.1 默认参数

C 语言**不支持**默认参数（C++ 支持）。

替代方案：

```c
// 用宏或重载函数名模拟
int add_default(int a, int b, int c) {
    return a + b + c;
}

#define add2(a, b) add_default(a, b, 0)
```

---

## 5.2 可变参数（variadic）

```c
#include <stdarg.h>

int sum(int count, ...) {
    va_list args;
    va_start(args, count);

    int total = 0;
    for (int i = 0; i < count; i++) {
        total += va_arg(args, int);
    }

    va_end(args);
    return total;
}

int main() {
    printf("%d\n", sum(3, 1, 2, 3));      // 6
    printf("%d\n", sum(5, 1, 2, 3, 4, 5)); // 15
    return 0;
}
```

可变参数步骤：

```text
1. va_list 声明参数列表
2. va_start 初始化
3. va_arg 逐个读取参数
4. va_end 清理
```

`printf` 就是可变参数函数的典型例子：

```c
int printf(const char *format, ...);
```

---

## 5.3 参数求值顺序

```c
// 未定义行为：求值顺序不确定
int result = f() + g();

// 避免依赖求值顺序
int a = f();
int b = g();
int result = a + b;
```

---

# 6. 函数指针

## 6.1 定义与使用

```c
int add(int a, int b) {
    return a + b;
}

int main() {
    // 声明函数指针
    int (*fp)(int, int);

    // 指向函数
    fp = add;

    // 调用
    printf("%d\n", fp(3, 5));   // 8
    return 0;
}
```

语法解析：

```text
int (*fp)(int, int)
     ^^  ^^^^^^^^^^^
     |   |
     |   参数列表
     函数指针名

返回类型是 int
```

---

## 6.2 函数指针数组

```c
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int mul(int a, int b) { return a * b; }
int div_(int a, int b) { return a / b; }

int (*ops[4])(int, int) = {add, sub, mul, div_};

int main() {
    printf("%d\n", ops[0](10, 5));   // 15
    printf("%d\n", ops[1](10, 5));   // 5
    printf("%d\n", ops[2](10, 5));   // 50
    printf("%d\n", ops[3](10, 5));   // 2
    return 0;
}
```

---

## 6.3 回调函数（Callback）

回调函数是通过函数指针调用的函数。

### 示例：排序比较器

```c
int ascending(const void *a, const void *b) {
    return *(int*)a - *(int*)b;
}

int descending(const void *a, const void *b) {
    return *(int*)b - *(int*)a;
}

int main() {
    int arr[] = {3, 1, 4, 1, 5};

    qsort(arr, 5, sizeof(int), ascending);   // 升序
    qsort(arr, 5, sizeof(int), descending);  // 降序
    return 0;
}
```

### 示例：事件处理

```c
typedef void (*Callback)(int);

void on_click(int x) {
    printf("Clicked at %d\n", x);
}

void on_move(int x) {
    printf("Moved to %d\n", x);
}

void trigger(int x, Callback cb) {
    cb(x);
}

int main() {
    trigger(10, on_click);   // Clicked at 10
    trigger(20, on_move);    // Moved to 20
    return 0;
}
```

---

## 6.4 函数指针作为返回值

```c
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }

// 返回函数指针的函数
int (*get_op(char op))(int, int) {
    if (op == '+') return add;
    if (op == '-') return sub;
    return NULL;
}

int main() {
    int (*fp)(int, int) = get_op('+');
    printf("%d\n", fp(3, 5));   // 8
    return 0;
}
```

**语法技巧**：使用 typedef 简化：

```c
typedef int (*BinOp)(int, int);

BinOp get_op(char op) {
    if (op == '+') return add;
    if (op == '-') return sub;
    return NULL;
}
```

---

## 6.5 函数指针与 typedef

```c
// 原始写法
int (*fp)(int, int);

// typedef 简化
typedef int (*BinOp)(int, int);
BinOp fp;   // 等价于 int (*fp)(int, int)
```

---

## 6.6 函数指针的常见应用

```text
• 回调函数（qsort、事件处理）
• 状态机（不同状态对应不同函数）
• 命令表（菜单项映射到函数）
• 插件系统（动态加载函数）
• 策略模式（运行时选择算法）
```

---

# 7. 递归函数

## 7.1 基本递归

```c
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int main() {
    printf("%d\n", factorial(5));   // 120
    return 0;
}
```

---

## 7.2 递归的两个要素

```text
• 基准情况（base case）：停止递归的条件
• 递归情况（recursive case）：调用自身，问题规模缩小
```

---

## 7.3 递归示例

斐波那契数列：

```c
int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}
```

**注意**：这种写法效率极低（指数级），应使用迭代或记忆化。

递归求和：

```c
int sum_array(int arr[], int n) {
    if (n == 0) return 0;
    return arr[n - 1] + sum_array(arr, n - 1);
}
```

---

## 7.4 递归 vs 迭代

| 特性 | 递归 | 迭代 |
| ---- | ---- | ---- |
| 代码简洁度 | 高 | 低 |
| 性能 | 较慢（函数调用开销） | 较快 |
| 栈空间 | 可能溢出 | 无额外开销 |
| 适用场景 | 树、分治 | 简单循环 |

---

# 8. 函数的作用域与存储类

## 8.1 局部函数（static）

```c
// file1.c
static void helper() {      // 仅本文件可见
    printf("helper\n");
}

void public_func() {
    helper();   // 可以调用
}
```

---

## 8.2 extern 函数

```c
// file2.c
extern void public_func();  // 声明外部函数

int main() {
    public_func();   // 调用 file1.c 中的函数
    return 0;
}
```

---

## 8.3 inline 函数

```c
inline int square(int x) {
    return x * x;
}
```

特点：

```text
• 建议编译器内联展开
• 消除函数调用开销
• 适用于短小、频繁调用的函数
• 只是建议，编译器可忽略
```

---

# 9. 函数与多文件编程

## 9.1 头文件声明

`mymath.h`：

```c
#ifndef MYMATH_H
#define MYMATH_H

int add(int a, int b);
int sub(int a, int b);
double average(double x, double y);

#endif
```

---

## 9.2 源文件实现

`mymath.c`：

```c
#include "mymath.h"

int add(int a, int b) {
    return a + b;
}

int sub(int a, int b) {
    return a - b;
}

double average(double x, double y) {
    return (x + y) / 2.0;
}
```

---

## 9.3 主程序使用

`main.c`：

```c
#include <stdio.h>
#include "mymath.h"

int main() {
    printf("%d\n", add(3, 5));
    printf("%.2f\n", average(3.0, 5.0));
    return 0;
}
```

编译：

```bash
gcc main.c mymath.c -o main
```

或分步：

```bash
gcc -c main.c -o main.o
gcc -c mymath.c -o mymath.o
gcc main.o mymath.o -o main
```

---

# 10. 函数的常见问题

## 1. 隐式声明

```c
int main() {
    int result = add(3, 5);   // 警告：隐式声明
    return 0;
}

int add(int a, int b) { return a + b; }
```

解决：在使用前声明：

```c
int add(int a, int b);   // 前置声明

int main() {
    int result = add(3, 5);
    return 0;
}

int add(int a, int b) { return a + b; }
```

---

## 2. 返回局部变量地址

```c
int* bad() {
    int a = 10;
    return &a;      // 危险
}
```

---

## 3. 数组参数 sizeof 陷阱

```c
void func(int arr[]) {
    printf("%zu\n", sizeof(arr));   // 8（指针大小），不是数组大小
}
```

解决：额外传长度：

```c
void func(int arr[], int size) { }
```

---

## 4. 忘记 return

```c
int add(int a, int b) {
    // 忘记 return
}

// 未定义行为
```

编译警告：`-Wall -Wextra`。

---

## 5. 函数名与变量名冲突

```c
int add = 10;         // 变量
int add(int a, int b); // 函数：冲突
```

---

## 6. 可变参数类型错误

```c
// 错误：va_arg 类型不匹配
va_arg(args, int);   // 实际传入的是 double
```

**注意**：可变参数不做类型检查，需确保类型一致。

---

# 11. 函数设计原则

## 11.1 单一职责

```text
• 一个函数只做一件事
• 函数体不宜过长（建议 < 50 行）
• 参数不宜过多（建议 ≤ 5 个）
```

---

## 11.2 命名清晰

```c
// 好
int calculate_average(int *arr, int size);
void print_user_info(const User *user);

// 不好
int calc(int *a, int n);
void print(...);
```

---

## 11.3 参数设计

```text
• 输入参数在前，输出参数在后
• 大结构体用 const 指针传递
• 需要修改的参数用指针
• 返回错误码用 int（0 成功，非 0 失败）
```

示例：

```c
int read_file(const char *path, char *buffer, int size);
```

---

## 11.4 错误处理

```c
// 返回错误码
int divide(int a, int b, int *result) {
    if (b == 0) return -1;    // 失败
    *result = a / b;
    return 0;                 // 成功
}

// 调用
int result;
if (divide(10, 0, &result) != 0) {
    printf("错误\n");
}
```

---

# 12. 常用标准库函数

## 12.1 字符串函数

```c
#include <string.h>

strlen(s);           // 长度
strcpy(dest, src);   // 复制
strncpy(dest, src, n);
strcat(dest, src);   // 拼接
strcmp(s1, s2);      // 比较
strchr(s, c);        // 查找字符
strstr(s, sub);      // 查找子串
```

---

## 12.2 数学函数

```c
#include <math.h>

sqrt(x);             // 平方根
pow(x, y);           // 幂
fabs(x);             // 绝对值
sin(x), cos(x);      // 三角函数
floor(x), ceil(x);   // 取整
```

---

## 12.3 内存函数

```c
#include <string.h>

memcpy(dest, src, n);    // 复制内存
memset(ptr, value, n);   // 设置内存
memcmp(a, b, n);         // 比较内存
```

---

## 12.4 工具函数

```c
#include <stdlib.h>

malloc(size);
calloc(n, size);
realloc(ptr, size);
free(ptr);
atoi(s);             // 字符串转 int
atof(s);             // 字符串转 double
qsort(arr, n, size, cmp);   // 排序
```

---

# 13. 推荐学习路线

```text
函数定义与调用
↓
函数声明与原型
↓
void 函数与返回值
↓
参数传递（值传递、指针传递）
↓
数组参数与二维数组参数
↓
结构体参数（值、指针、const）
↓
指针函数（返回指针）
↓
函数指针与回调
↓
递归
↓
可变参数
↓
多文件编程
↓
综合项目实践
```

---

# 14. 推荐练习项目

## 初级

* 计算器（加减乘除函数）
* 数组排序（冒泡、选择）
* 字符串处理函数（自己实现 strlen、strcpy）

---

## 中级

* 通用排序函数（qsort + 回调）
* 状态机（函数指针实现）
* 通讯录管理（结构体 + 文件）

---

## 高级

* 简易解释器（函数表 + 命令分发）
* 插件系统（动态加载函数）
* 事件驱动框架（回调注册）

---

# 15. 核心语法速查

```c
// 函数定义
int add(int a, int b) {
    return a + b;
}

// 函数声明
int add(int a, int b);

// void 函数
void print_hello(void) {
    printf("Hello\n");
}

// 指针参数
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// 数组参数
void print_arr(int arr[], int size) { }

// 二维数组参数
void print_mat(int mat[][4], int rows) { }

// 结构体参数
void move(Point *p, int dx, int dy) { }

// const 指针参数
void print(const Point *p) { }

// 二级指针参数
void alloc(int **p, int n) { }

// 指针函数（返回指针）
int* find_max(int *arr, int size) { }

// 函数指针
int (*fp)(int, int) = add;

// 函数指针作为参数
int calculate(int a, int b, int (*op)(int, int)) { }

// 函数指针数组
int (*ops[4])(int, int) = {add, sub, mul, div_};

// typedef 简化
typedef int (*BinOp)(int, int);

// 递归
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// 可变参数
int sum(int count, ...) {
    va_list args;
    va_start(args, count);
    // ...
    va_end(args);
}

// 多文件编程
#include "mymath.h"   // 头文件声明
gcc main.c mymath.c -o main
```